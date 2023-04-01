#include "utils/Hast.h"
#include <sstream>

#include "R/Protect.h"
#include "utils/Pool.h"

#include "utils/serializerData.h"
#include "utils/SerializerDebug.h"
#include "dirent.h"
#include "api.h"
#include <bits/stdc++.h>

#define PRINT_HAST_SRC_ENTRIES 0

namespace rir {

std::set<SEXP> Hast::blacklist;
std::unordered_map<SEXP, HastData> Hast::hastMap;
std::unordered_map<unsigned, HastInfo> Hast::sPoolHastMap;
std::unordered_map<unsigned, HastInfo> Hast::cPoolHastMap;

std::unordered_map<SEXP, HastInfo> Hast::cPoolInverseMap;
std::unordered_map<SEXP, HastInfo> Hast::sPoolInverseMap;

std::unordered_map<Opcode*, std::set<LastDispatchFastcaseHolder*>> Hast::l2FastcaseInvalidationCache;
std::unordered_map<int, SEXP> Hast::debugMap;
int Hast::debugIdx;

static size_t charToInt(const char* p, size_t & hast) {
    for (size_t i = 0; i < strlen(p); ++i) {
        hast = ((hast << 5) + hast) + p[i];
    }
    return hast;
}

void Hast::printHastSrcData(DispatchTable* vtable, SEXP parentHast) {
    DispatchTable * currVtab = vtable;
    unsigned indexOffset = 0;

    auto addSrcToMap = [&] (const unsigned & src, bool sourcePool = true) {
        std::cout << (sourcePool ? "spool: ": "cpool: ") << src << std::endl;
    };

    std::function<void(Code *, Function *)> iterateOverCodeObjs = [&] (Code * c, Function * funn) {
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
                indexOffset++;
            }

            // call sites
            switch (bc.bc) {
                case Opcode::push_:
                    addSrcToMap(bc.immediate.pool, false);
                    indexOffset++;
                    break;
                case Opcode::call_:
                case Opcode::named_call_:
                    addSrcToMap(bc.immediate.callFixedArgs.ast, false);
                    indexOffset++;
                    break;
                case Opcode::call_dots_:
                    addSrcToMap(bc.immediate.callFixedArgs.ast, false);
                    indexOffset++;
                    break;
                case Opcode::call_builtin_:
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

void Hast::populateHastSrcData(DispatchTable* vtable, SEXP parentHast) {
    DispatchTable * currVtab = vtable;
    unsigned indexOffset = 0;

    auto addSrcToMap = [&] (const unsigned & src, bool sourcePool = true) {
        if (sourcePool) {
            sPoolHastMap[src] = {parentHast, indexOffset, src};
            sPoolInverseMap[src_pool_at(src)] = sPoolHastMap[src];
        } else {
            cPoolHastMap[src] = {parentHast, indexOffset, src};
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
                case Opcode::push_:
                    #if PRINT_HAST_SRC_ENTRIES == 1
                    std::cout << "src_hast_entry[push_] at indexOffset: " << indexOffset << "," << bc.immediate.pool << " [" << Pool::get(bc.immediate.pool) << "]" << std::endl;
                    #endif
                    addSrcToMap(bc.immediate.pool, false);
                    indexOffset++;
                    break;
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

#define simple_as_environment(arg) (IS_S4_OBJECT(arg) && (TYPEOF(arg) == S4SXP) ? R_getS4DataSlot(arg, ENVSXP) : arg)

static void getHastPrefix(SEXP env, std::stringstream & qHast) {
    if (TYPEOF(env) == ENVSXP) {
        if (env == R_GlobalEnv) {
            qHast << "GE:";
        } else if (env == R_BaseEnv) {
            qHast << "BE:";
        } else if (env == R_EmptyEnv) {
            qHast << "EE:";
        } else if (R_IsPackageEnv(env)) {
            qHast << "PE:" << Rf_translateChar(STRING_ELT(R_PackageEnvName(env), 0)) << ":";
        } else if (R_IsNamespaceEnv(env)) {
            qHast << "NS:" << Rf_translateChar(STRING_ELT(R_NamespaceEnvSpec(env), 0)) << ":";
        } else {
            qHast << "AE:";
            getHastPrefix(ENCLOS(env), qHast);
        }
    }
}

SEXP Hast::getHast(SEXP body, SEXP formals, SEXP env) {
    static bool skipGlobalBc = getenv("SKIP_GLOBAL_BC") ? getenv("SKIP_GLOBAL_BC")[0] == '1' : false;
    if (skipGlobalBc && env == R_GlobalEnv) return R_NilValue;
    std::stringstream qHast;
    getHastPrefix(env, qHast);
    if (qHast.str().size() == 0) return R_NilValue;
    size_t hast = 0;
    hash_ast(body, hast);
    hash_ast(formals, hast);
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
                case Opcode::push_:
                    if (indexOffset == requiredOffset) {
                        // required thing was constant pool index
                        poolIdx = bc.immediate.pool;
                        done = true;
                        return;
                    }
                    indexOffset++;
                    break;

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

rir::DispatchTable * Hast::getVtableObjectAtOffset(SEXP hastSym, int offset) {
    SEXP vtabContainer = hastMap[hastSym].vtabContainer;
    if (!vtabContainer) {
        Rf_error("getVtableObjectAtOffset failed!");
    }
    if (!DispatchTable::check(vtabContainer)) {
        Rf_error("getVtableObjectAtOffset vtable corrupted");
    }
    auto vtab = DispatchTable::unpack(vtabContainer);
    auto r = getResultAtOffset(vtab, offset);

    return r.vtable;
}

rir::DispatchTable * Hast::getVtableObjectAtOffset(rir::DispatchTable * vtab, int offset) {
    auto r = getResultAtOffset(vtab, offset);
    return r.vtable;
}

rir::Code * Hast::getCodeObjectAtOffset(SEXP hastSym, int offset) {
    SEXP vtabContainer = hastMap[hastSym].vtabContainer;
    if (!vtabContainer) {
        Rf_error("getCodeObjectAtOffset failed!");
    }
    if (!DispatchTable::check(vtabContainer)) {
        Rf_error("getCodeObjectAtOffset vtable corrupted");
    }
    auto vtab = DispatchTable::unpack(vtabContainer);
    auto r = getResultAtOffset(vtab, offset);

    return r.code;
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

void Hast::printRawFeedback(const DispatchTable* vtab, std::ostream& out, const int & space) {
    int idx = 0;
    const DispatchTable* currVtab = vtab;

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
                    out << "[" << idx++ << "] record_call_: ";
                    bc.print(out);
                }

                if (bc.bc == Opcode::record_test_) {
                    out << "[" << idx++ << "] record_test_: ";
                    bc.print(out);
                }

                if (bc.bc == Opcode::record_type_) {
                    out << "[" << idx++ << "] record_type_: ";
                    bc.print(out);
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

L2ContextData Hast::getSpeculativeContext(const DispatchTable* vtab, const int & offset) {
    // std::pair<rir::Code*,Opcode*> res;
    // res.first = nullptr;
    // res.second = nullptr;
    int idx = 0;
    const DispatchTable* currVtab = vtab;
    Protect protecc;
    std::vector<SEXP> sContextVector;

    L2ContextData res;

    std::function<void(Code*, Function*)> iterateOverCodeObjs =
        [&](Code* c, Function* funn) {
            // Default args
            if (funn) {
                auto nargs = funn->nargs();
                for (unsigned i = 0; i < nargs; i++) {
                    auto code = funn->defaultArg(i);
                    if (code != nullptr) {
                        iterateOverCodeObjs(code, nullptr);
                        if (res.tag != 99) return;
                    }
                }
            }

            {
                if (idx == offset) {
                    res.tag = 4;
                    res.code = c;
                    res.pc = nullptr;
                    return;
                }
                idx++;
            }

            {
                if (idx == offset) {
                    res.tag = 3;
                    res.code = c;
                    res.pc = nullptr;
                    return;
                }
                idx++;
            }

            Opcode* pc = c->code();
            std::vector<BC::FunIdx> promises;
            Protect p;
            while (pc < c->endCode()) {
                BC bc = BC::decode(pc, c);
                bc.addMyPromArgsTo(promises);
                if (bc.bc == Opcode::record_call_) {
                    if (idx == offset) {
                        res.tag = 2;
                        res.code = c;
                        res.pc = pc;
                        return;
                    }
                    idx++;
                }

                if (bc.bc == Opcode::record_test_) {
                    if (idx == offset) {
                        res.tag = 1;
                        res.code = c;
                        res.pc = pc;
                        return;
                    }
                    idx++;
                }

                if (bc.bc == Opcode::record_type_) {
                    if (idx == offset) {
                        res.tag = 0;
                        res.code = c;
                        res.pc = pc;
                        return;
                    }
                    idx++;
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
                        if (res.tag != 99) return;
                    }
                }

                pc = BC::next(pc);
            }

            // Iterate over promises code objects recursively
            for (auto i : promises) {
                auto prom = c->getPromise(i);
                iterateOverCodeObjs(prom, nullptr);
                if (res.tag != 99) return;
            }
        };
    Code* genesisCodeObj = currVtab->baseline()->body();
    Function* genesisFunObj = genesisCodeObj->function();
    iterateOverCodeObjs(genesisCodeObj, genesisFunObj);

    assert(res.tag != 99);
    return res;
}

void Hast::addSpeculativeContext(SEXP mContainer, const DispatchTable* vtab, std::set<uintptr_t> pods) {
    // int idx = 0;
    const DispatchTable* currVtab = vtab;
    Protect protecc;
    std::vector<SEXP> sContextVector;

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

            {
                // Add function flags
                SEXP store;
                protecc(store = Rf_allocVector(VECSXP, speculativeContextElement::getStorageSize()));
                speculativeContextElement::addTag(store, 4);
                speculativeContextElement::addPC(store, (uintptr_t) 0);
                speculativeContextElement::addPOD(store, false);
                speculativeContextElement::addVal(store, c->function()->flags.to_i());
                sContextVector.push_back(store);
            }

            {
                // Add code flags
                SEXP store;
                protecc(store = Rf_allocVector(VECSXP, speculativeContextElement::getStorageSize()));
                speculativeContextElement::addTag(store, 3);
                speculativeContextElement::addPC(store, (uintptr_t) 0);
                speculativeContextElement::addPOD(store, false);
                speculativeContextElement::addVal(store, c->flags.to_i());
                sContextVector.push_back(store);
            }

            Opcode* pc = c->code();
            std::vector<BC::FunIdx> promises;
            Protect p;
            while (pc < c->endCode()) {
                BC bc = BC::decode(pc, c);
                bc.addMyPromArgsTo(promises);

                bool pod = pods.find((uintptr_t) pc) != pods.end();

                SEXP store;
                if (bc.bc == Opcode::record_call_) {
                    protecc(store = Rf_allocVector(VECSXP, speculativeContextElement::getStorageSize()));
                    speculativeContextElement::addTag(store, 2);
                    speculativeContextElement::addPC(store, (uintptr_t) pc);
                    speculativeContextElement::addPOD(store, pod);
                    ObservedCallees * feedback = (ObservedCallees *) (pc + 1);
                    if (feedback->numTargets == 1) {
                        auto target = feedback->getTarget(c, 0);
                        if (TYPEOF(target) == CLOSXP && TYPEOF(BODY(target)) == EXTERNALSXP && DispatchTable::check(BODY(target))) {
                            auto targetVtab = DispatchTable::unpack(BODY(target));
                            auto targetCode = targetVtab->baseline()->body();

                            auto hastInfo = Hast::getHastInfo(targetCode->src, true);
                            if (hastInfo.isValid()) {
                                speculativeContextElement::addVal(store, hastInfo.hast);
                            } else {
                                speculativeContextElement::addVal(store, Rf_ScalarInteger(-1));
                            }
                        } else if (TYPEOF(target) == SPECIALSXP) {
                            speculativeContextElement::addVal(store, Rf_ScalarInteger(target->u.primsxp.offset));
                        } else {
                            speculativeContextElement::addVal(store, Rf_ScalarInteger(-2));
                        }
                    } else {
                        speculativeContextElement::addVal(store, Rf_ScalarInteger(-3));
                    }
                    sContextVector.push_back(store);
                    // bc.print(std::cout);
                    // speculativeContextElement::print(store);
                    // std::cout << std::endl;
                }

                if (bc.bc == Opcode::record_test_) {
                    protecc(store = Rf_allocVector(VECSXP, speculativeContextElement::getStorageSize()));
                    speculativeContextElement::addTag(store, 1);
                    speculativeContextElement::addPC(store, (uintptr_t) pc);
                    speculativeContextElement::addPOD(store, pod);
                    ObservedTest* feedback = (ObservedTest*)(pc + 1);
                    speculativeContextElement::addVal(store, *(reinterpret_cast<uint32_t*>(feedback)));
                    sContextVector.push_back(store);
                    // bc.print(std::cout);
                    // speculativeContextElement::print(store);
                    // std::cout << std::endl;
                }

                if (bc.bc == Opcode::record_type_) {
                    protecc(store = Rf_allocVector(VECSXP, speculativeContextElement::getStorageSize()));
                    speculativeContextElement::addTag(store, 0);
                    speculativeContextElement::addPC(store, (uintptr_t) pc);
                    speculativeContextElement::addPOD(store, pod);
                    ObservedValues* feedback = (ObservedValues*)(pc + 1);
                    uint32_t storedVal = *((uint32_t*) feedback);
                    speculativeContextElement::addVal(store, storedVal);
                    sContextVector.push_back(store);

                    // std::cout << ">>>>>>>>>>>>>>>>" << std::endl;
                    // std::cout << "feedback: " << *((uint32_t*) feedback) << std::endl;
                    // std::cout << "Storing Val: " << storedVal << std::endl;
                    // speculativeContextElement::print(store);
                    // std::cout << std::endl;
                    // bc.print(std::cout);
                    // ObservedValues test;
                    // *(&test) = *((ObservedValues *)&storedVal);
                    // std::cout << "   [ ";
                    // test.print(std::cout);
                    // std::cout << " ]" << std::endl;
                    // std::cout << "<<<<<<<<<<<<<<<<" << std::endl;
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
    SEXP conStore;
    protecc(conStore = Rf_allocVector(VECSXP, sContextVector.size()));
    for(size_t i = 0; i < sContextVector.size(); i++) {
        SET_VECTOR_ELT(conStore, i, sContextVector[i]);

    }
    SET_VECTOR_ELT(mContainer, 1, conStore);
}

void Hast::serializerCleanup() {
    auto prefix = getenv("PIR_SERIALIZE_PREFIX") ? getenv("PIR_SERIALIZE_PREFIX") : "bitcodes";

    std::stringstream savePath;
    savePath << prefix << "/";

    DIR *dir;
    struct dirent *ent;


    if ((dir = opendir (savePath.str().c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            std::string fName = ent->d_name;
            if (fName.find(".meta") != std::string::npos) {
                Protect protecc;
                std::stringstream metaPath;
                metaPath << prefix << "/" << fName;

                FILE *reader;
                reader = fopen(metaPath.str().c_str(),"r");

                if (!reader) {
                    SerializerDebug::infoMessage("(*) serializer cleanup failed", 0);
                    continue;
                }

                SEXP result;
                protecc(result= R_LoadFromFile(reader, 0));
                fclose(reader);

                // check if the currentHast is blacklisted
                SEXP hast = serializerData::getHast(result);

                if (blacklist.count(hast) > 0) {
                    const int removeRes = remove(metaPath.str().c_str());
                    if( removeRes != 0 ){
                        std::cerr << "[Warning] Failed to remove: " << metaPath.str() << ", res: " << removeRes << std::endl;
                    }
                    continue;
                }

                // Todo, blacklist specific contexts instead of the whole file...
                bool err = false;
                serializerData::iterate(result, [&](SEXP offsetSym, SEXP conSym, SEXP cData, bool isMask) {
                    if (!isMask) {
                        auto rData = contextData::getReqMapAsVector(cData);
                        for (int i = 0; i < Rf_length(rData); i++) {
                            SEXP dep = VECTOR_ELT(rData, i);
                            if (blacklist.count(dep) > 0) {
                                err = true;
                            }
                        }
                    }
                });

                if (err) {
                    const int removeRes = remove(metaPath.str().c_str());
                    if( removeRes != 0 ){
                        std::cerr << "Failed to remove: " << metaPath.str() << ", res: " << removeRes << std::endl;
                    }
                }
            }
        }
        closedir (dir);
    }
}

} // namespace rir
