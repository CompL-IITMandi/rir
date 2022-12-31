#include "utils/Hast.h"
#include <sstream>

#include "R/Protect.h"
#include "utils/Pool.h"

#include "utils/serializerData.h"

#define PRINT_HAST_SRC_ENTRIES 0

namespace rir {

std::unordered_map<SEXP, HastData> Hast::hastMap;
std::unordered_map<unsigned, HastInfo> Hast::sPoolHastMap;
std::unordered_map<unsigned, HastInfo> Hast::cPoolHastMap;

std::unordered_map<SEXP, HastInfo> Hast::cPoolInverseMap;

static size_t charToInt(const char* p, size_t & hast) {
    for (size_t i = 0; i < strlen(p); ++i) {
        hast = ((hast << 5) + hast) + p[i];
    }
    return hast;
}

void Hast::populateHastSrcData(DispatchTable* vtable, SEXP parentHast) {
    DispatchTable * currVtab = vtable;
    unsigned indexOffset = 0;

    auto addSrcToMap = [&] (const unsigned & src, bool sourcePool = true) {
        if (sourcePool) {
            sPoolHastMap[src] = {parentHast, indexOffset};
        } else {
            cPoolHastMap[src] = {parentHast, indexOffset};
            cPoolInverseMap[Pool::get(src)] = cPoolHastMap[src];
        }
    };

    std::function<void(Code *, Function *)> iterateOverCodeObjs = [&] (Code * c, Function * funn) {
        #if PRINT_HAST_SRC_ENTRIES == 1
        std::cout << "src_hast_entry[C] at indexOffset: " << indexOffset << "," << c->src << std::endl;
        #endif

        addSrcToMap(c->src);
        indexOffset++;
        if (funn) {
            auto nargs = funn->nargs();
            for (unsigned i = 0; i < nargs; i++) {
                auto code = funn->defaultArg(i);
                if (code != nullptr) {
                    iterateOverCodeObjs(code, nullptr);
                }
            }
        }

        Opcode* pc = c->code();
        std::vector<BC::FunIdx> promises;
        Protect p;
        while (pc < c->endCode()) {
            BC bc = BC::decode(pc, c);
            bc.addMyPromArgsTo(promises);

            // src code language objects
            unsigned s = c->getSrcIdxAt(pc, true);
            if (s != 0) {
                addSrcToMap(s);
                #if PRINT_HAST_SRC_ENTRIES == 1
                std::cout << "src_hast_entry[source_pool] at indexOffset: " << indexOffset << "," << s << std::endl;
                #endif
                indexOffset++;
            }

            // call sites
            switch (bc.bc) {
                case Opcode::call_:
                case Opcode::named_call_:
                    #if PRINT_HAST_SRC_ENTRIES == 1
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callFixedArgs.ast << " [" << Pool::get(bc.immediate.callFixedArgs.ast) << "]" << std::endl;
                    #endif
                    addSrcToMap(bc.immediate.callFixedArgs.ast, false);
                    indexOffset++;
                    break;
                case Opcode::call_dots_:
                    #if PRINT_HAST_SRC_ENTRIES == 1
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callFixedArgs.ast << " [" << Pool::get(bc.immediate.callFixedArgs.ast) << "]" << std::endl;
                    #endif
                    addSrcToMap(bc.immediate.callFixedArgs.ast, false);
                    indexOffset++;
                    break;
                case Opcode::call_builtin_:
                    #if PRINT_HAST_SRC_ENTRIES == 1
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callBuiltinFixedArgs.ast << "[ " << Pool::get(bc.immediate.callFixedArgs.ast) << "]" << std::endl;
                    #endif
                    addSrcToMap(bc.immediate.callBuiltinFixedArgs.ast, false);
                    indexOffset++;
                    break;
                default: {}
            }

            // inner functions
            if (bc.bc == Opcode::push_ && TYPEOF(bc.immediateConst()) == EXTERNALSXP) {
                SEXP iConst = bc.immediateConst();
                if (DispatchTable::check(iConst)) {
                    currVtab = DispatchTable::unpack(iConst);
                    auto c = currVtab->baseline()->body();
                    auto f = c->function();
                    iterateOverCodeObjs(c, f);
                }
            }

            pc = BC::next(pc);
        }

        // Iterate over promises code objects recursively
        for (auto i : promises) {
            auto prom = c->getPromise(i);
            iterateOverCodeObjs(prom, nullptr);
        }
    };

    Code * genesisCodeObj = currVtab->baseline()->body();
    Function * genesisFunObj = currVtab->baseline()->body()->function();
    iterateOverCodeObjs(genesisCodeObj, genesisFunObj);
}

static void hash_ast(SEXP ast, size_t & hast) {
    int len = Rf_length(ast);
    int type = TYPEOF(ast);

    if (type == SYMSXP) {
        const char * pname = CHAR(PRINTNAME(ast));
        hast = hast * 31;
        charToInt(pname, hast);
    } else if (type == STRSXP) {
        const char * pname = CHAR(STRING_ELT(ast, 0));
        hast = hast * 31;
        charToInt(pname, hast);
    } else if (type == LGLSXP) {
        for (int i = 0; i < len; i++) {
            int ival = LOGICAL(ast)[i];
            hast += ival;
        }
    } else if (type == INTSXP) {
        for (int i = 0; i < len; i++) {
            int ival = INTEGER(ast)[i];
            hast += ival;
        }
    } else if (type == REALSXP) {
        for (int i = 0; i < len; i++) {
            double dval = REAL(ast)[i];
            hast += dval;
        }
    } else if (type == LISTSXP || type == LANGSXP) {
        hast *= 31;
        hash_ast(CAR(ast), ++hast);
        hast *= 31;
        hash_ast(CDR(ast), ++hast);
    }
}

bool Hast::isAnonEnv(SEXP env) {
    SEXP x = env;
    if (x == R_GlobalEnv) {
        return false;
    } else if (x == R_BaseEnv) {
        return false;
    } else if (x == R_EmptyEnv) {
        return false;
    } else if (R_IsPackageEnv(x)) {
        return false;
    } else if (R_IsNamespaceEnv(x)) {
        return false;
    } else {
        return true;
    }
}

SEXP Hast::getHast(SEXP body, SEXP env) {
    std::stringstream qHast;
    static bool skipGlobalBc = getenv("SKIP_GLOBAL_BC") ? true : false;
    SEXP x = env;
    if (x == R_GlobalEnv) {
        if (skipGlobalBc) {
            return R_NilValue;
        }
        qHast << "GE:";
    } else if (x == R_BaseEnv) {
        qHast << "BE:";
    } else if (x == R_EmptyEnv) {
        qHast << "EE:";
    } else if (R_IsPackageEnv(x)) {
        qHast << "PE:" << Rf_translateChar(STRING_ELT(R_PackageEnvName(x), 0)) << ":";
    } else if (R_IsNamespaceEnv(x)) {
        qHast << "NS:" << Rf_translateChar(STRING_ELT(R_NamespaceEnvSpec(x), 0)) << ":";
    } else {
        return R_NilValue;
        qHast << "AE:";
    }
    size_t hast = 0;
    hash_ast(body, hast);
    qHast << hast;
    SEXP calcHast = Rf_install(qHast.str().c_str());
    return calcHast;
}

struct TraversalResult {
    Code * code;
    DispatchTable * vtable;
    unsigned srcIdx;
};

static inline TraversalResult getResultAtOffset(DispatchTable * vtab, const unsigned & requiredOffset) {
    bool done = false;
    unsigned indexOffset = 0;

    DispatchTable * currVtab = vtab;
    Code * currCode = nullptr;
    unsigned poolIdx = currVtab->baseline()->body()->src;

    std::function<void(Code *, Function *)> iterateOverCodeObjs = [&] (Code * c, Function * funn) {
        if (done) return;
        if (indexOffset == requiredOffset) {
            // required thing was a code obj or a dispatch table obj
            poolIdx = c->src;
            currCode = c;
            done = true;
            return;
        }
        indexOffset++;

        // Default args
        if (funn) {
            auto nargs = funn->nargs();
            for (unsigned i = 0; i < nargs; i++) {
                auto code = funn->defaultArg(i);
                if (code != nullptr) {
                    iterateOverCodeObjs(code, nullptr);
                    if (done) return;
                }
            }
        }

        Opcode* pc = c->code();
        std::vector<BC::FunIdx> promises;
        Protect p;
        while (pc < c->endCode()) {
            if (done) return;
            BC bc = BC::decode(pc, c);
            bc.addMyPromArgsTo(promises);

            // src code language objects
            unsigned s = c->getSrcIdxAt(pc, true);
            if (s != 0) {
                if (indexOffset == requiredOffset) {
                    // required thing was src pool index
                    poolIdx = s;
                    done = true;
                    return;
                }
                indexOffset++;
            }

            // call sites
            switch (bc.bc) {
                case Opcode::call_:
                case Opcode::named_call_:
                    if (indexOffset == requiredOffset) {
                        // required thing was constant pool index
                        poolIdx = bc.immediate.callFixedArgs.ast;
                        done = true;
                        return;
                    }
                    indexOffset++;
                    break;
                case Opcode::call_dots_:
                    if (indexOffset == requiredOffset) {
                        // required thing was constant pool index
                        poolIdx = bc.immediate.callFixedArgs.ast;
                        done = true;
                        return;
                    }
                    indexOffset++;
                    break;
                case Opcode::call_builtin_:
                    if (indexOffset == requiredOffset) {
                        // required thing was constant pool index
                        poolIdx = bc.immediate.callBuiltinFixedArgs.ast;
                        done = true;
                        return;
                    }
                    indexOffset++;
                    break;
                default: {}
            }

            // inner functions
            if (bc.bc == Opcode::push_ && TYPEOF(bc.immediateConst()) == EXTERNALSXP) {
                SEXP iConst = bc.immediateConst();
                if (DispatchTable::check(iConst)) {
                    currVtab = DispatchTable::unpack(iConst);
                    auto c = currVtab->baseline()->body();
                    auto f = c->function();
                    iterateOverCodeObjs(c, f);
                    if (done) return;
                }
            }

            pc = BC::next(pc);
        }

        // Iterate over promises code objects recursively
        for (auto i : promises) {
            auto prom = c->getPromise(i);
            iterateOverCodeObjs(prom, nullptr);
            if (done) return;
        }
    };

    Code * genesisCodeObj = currVtab->baseline()->body();
    Function * genesisFunObj = genesisCodeObj->function();

    iterateOverCodeObjs(genesisCodeObj, genesisFunObj);

    TraversalResult r;
    r.code = currCode;
    r.vtable = currVtab;
    r.srcIdx = poolIdx;
    return r;
}


unsigned Hast::getSrcPoolIndexAtOffset(SEXP hastSym, int requiredOffset) {
    SEXP vtabContainer = hastMap[hastSym].vtabContainer;
    if (!vtabContainer) {
        Rf_error("getSrcPoolIndexAtOffset failed!");
    }
    if (!DispatchTable::check(vtabContainer)) {
        Rf_error("getSrcPoolIndexAtOffset vtable corrupted");
    }
    auto vtab = DispatchTable::unpack(vtabContainer);
    auto r = getResultAtOffset(vtab, requiredOffset);

    return r.srcIdx;
}

void Hast::populateTypeFeedbackData(SEXP container, DispatchTable * vtab) {
    DispatchTable * currVtab = vtab;

    std::function<void(Code *, Function *)> iterateOverCodeObjs = [&] (Code * c, Function * funn) {
        // Default args
        if (funn) {
            auto nargs = funn->nargs();
            for (unsigned i = 0; i < nargs; i++) {
                auto code = funn->defaultArg(i);
                if (code != nullptr) {
                    iterateOverCodeObjs(code, nullptr);
                }
            }
        }

        Opcode* pc = c->code();
        std::vector<BC::FunIdx> promises;
        Protect p;
        while (pc < c->endCode()) {
            BC bc = BC::decode(pc, c);
            bc.addMyPromArgsTo(promises);

            // call sites
            if (bc.bc == Opcode::record_type_) {
                    // std::cout << "FOR CODE:  " << c << std::endl;
                    // std::cout << "record_type " << feedback << " [" << *((uint32_t *) feedback) << "]" << std::endl;
                    // std::cout << "  ";
                    // feedback->print(std::cout);
                    // std::cout << std::endl;
                    contextData::addObservedValueToVector(container, &bc.immediate.typeFeedback);
            }
            // switch (bc.bc) {
            //     case Opcode::record_type_:
            //         break;
            // }

            // inner functions
            if (bc.bc == Opcode::push_ && TYPEOF(bc.immediateConst()) == EXTERNALSXP) {
                SEXP iConst = bc.immediateConst();
                if (DispatchTable::check(iConst)) {
                    currVtab = DispatchTable::unpack(iConst);
                    auto c = currVtab->baseline()->body();
                    auto f = c->function();
                    iterateOverCodeObjs(c, f);
                }
            }

            pc = BC::next(pc);
        }

        // Iterate over promises code objects recursively
        for (auto i : promises) {
            auto prom = c->getPromise(i);
            iterateOverCodeObjs(prom, nullptr);
        }
    };

    Code * genesisCodeObj = currVtab->baseline()->body();
    Function * genesisFunObj = genesisCodeObj->function();

    iterateOverCodeObjs(genesisCodeObj, genesisFunObj);
}

// Handling call site information
void Hast::populateOtherFeedbackData(SEXP container, DispatchTable* vtab) {
    DispatchTable* currVtab = vtab;

    std::function<void(Code*, Function*)> iterateOverCodeObjs =
        [&](Code* c, Function* funn) {
            // Default args
            if (funn) {
                auto nargs = funn->nargs();
                for (unsigned i = 0; i < nargs; i++) {
                    auto code = funn->defaultArg(i);
                    if (code != nullptr) {
                        iterateOverCodeObjs(code, nullptr);
                    }
                }
            }

            Opcode* pc = c->code();
            std::vector<BC::FunIdx> promises;
            Protect p;
            while (pc < c->endCode()) {
                BC bc = BC::decode(pc, c);
                bc.addMyPromArgsTo(promises);

                if (bc.bc == Opcode::record_call_) {
                    // ObservedCallees * v = (ObservedCallees *) (pc + 1);
                    // std::cout << "Decoded target from pointer: [";
                    // for (auto & ele : v->targets) {
                    //     std::cout << ele << " ";
                    // }
                    // std::cout << "]" << std::endl;

                    ObservedCallees prof = bc.immediate.callFeedback;

                    // std::cout << "Decoded target from data: [";
                    // for (auto & ele : prof.targets) {
                    //     std::cout << ele << " ";
                    // }
                    // std::cout << "]" << std::endl;

                    contextData::addObservedCallSiteInfo(container, &prof, c);
                }

                if (bc.bc == Opcode::record_test_) {
                    contextData::addObservedTestToVector(container, &bc.immediate.testFeedback);
                }

                // inner functions
                if (bc.bc == Opcode::push_ &&
                    TYPEOF(bc.immediateConst()) == EXTERNALSXP) {
                    SEXP iConst = bc.immediateConst();
                    if (DispatchTable::check(iConst)) {
                        currVtab = DispatchTable::unpack(iConst);
                        auto c = currVtab->baseline()->body();
                        auto f = c->function();
                        iterateOverCodeObjs(c, f);
                    }
                }

                pc = BC::next(pc);
            }

            // Iterate over promises code objects recursively
            for (auto i : promises) {
                auto prom = c->getPromise(i);
                iterateOverCodeObjs(prom, nullptr);
            }
        };

    Code* genesisCodeObj = currVtab->baseline()->body();
    Function* genesisFunObj = genesisCodeObj->function();

    iterateOverCodeObjs(genesisCodeObj, genesisFunObj);
}



} // namespace rir
