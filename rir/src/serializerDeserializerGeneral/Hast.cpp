#include "Hast.h"
#include "R/r.h"

#include "serializerDeserializerGeneral/UMap.h"

#include "serializerDeserializerGeneral/General.h"

#define PRINT_HAST_SRC_ENTRIES 0
#define DEBUG_BLACKLIST_ENTRIES 0

namespace rir {

// If this is true only functions that belong to named namespaces will
// be serialized, other functions will be skipped i.e. Global Env funs.
// This is to prevent duplicate hasts in the users global environment.
// Ideally we should be able to get rid of this by qualifying the hast
// with the md5 of the file and its line number;
// but file ref might not always exists so we settle for this more
// conservative approach instead.
static bool skipGlobalBc = getenv("SKIP_GLOBAL_BC") ? true : false;

hastAndIndex getHastAndIndex(unsigned src, bool constantPool) {

    if (constantPool) {
        if (GeneralMaps::CPOOL_HAST_MAP.find(src) != GeneralMaps::CPOOL_HAST_MAP.end()) {
            auto ele = GeneralMaps::CPOOL_HAST_MAP[src];
            hastAndIndex res = { ele.first, ele.second };
            return res;
        }
    } else {
        if (GeneralMaps::SPOOL_HAST_MAP.find(src) != GeneralMaps::SPOOL_HAST_MAP.end()) {
            auto ele = GeneralMaps::SPOOL_HAST_MAP[src];
            hastAndIndex res = { ele.first, ele.second };
            return res;
        }
    }

    hastAndIndex res = { R_NilValue, 0 };
    return res;
}

void populateHastSrcData(DispatchTable* vtable, SEXP parentHast) {
    #if PRINT_HAST_SRC_ENTRIES == 1
    std::cout << "(R) Populating the hast-src data for " << CHAR(PRINTNAME(parentHast)) << std::endl;
    #endif
    DispatchTable * currVtab = vtable;
    unsigned indexOffset = 0;

    auto addSrcToMap = [&] (const unsigned & src, bool constantPool) {
        if (constantPool) {
            GeneralMaps::CPOOL_HAST_MAP[src] = std::pair<SEXP, unsigned>(parentHast, indexOffset);
        } else {
            GeneralMaps::SPOOL_HAST_MAP[src] = std::pair<SEXP, unsigned>(parentHast, indexOffset);
        }
    };

    std::function<void(Code *, Function *)> iterateOverCodeObjs = [&] (Code * c, Function * funn) {
        #if PRINT_HAST_SRC_ENTRIES == 1
        std::cout << "src_hast_entry[C] at indexOffset: " << indexOffset << "," << c->src << std::endl;
        #endif

        addSrcToMap(c->src, false);
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
        while (pc < c->endCode()) {
            BC bc = BC::decode(pc, c);
            bc.addMyPromArgsTo(promises);

            // src code language objects
            unsigned s = c->getSrcIdxAt(pc, true);
            if (s != 0) {
                addSrcToMap(s, false);
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
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callFixedArgs.ast << std::endl;
                    #endif
                    addSrcToMap(bc.immediate.callFixedArgs.ast, true);
                    indexOffset++;
                    break;
                case Opcode::call_dots_:
                    #if PRINT_HAST_SRC_ENTRIES == 1
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callFixedArgs.ast << std::endl;
                    #endif
                    addSrcToMap(bc.immediate.callFixedArgs.ast, true);
                    indexOffset++;
                    break;
                case Opcode::call_builtin_:
                    #if PRINT_HAST_SRC_ENTRIES == 1
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callBuiltinFixedArgs.ast << std::endl;
                    #endif
                    addSrcToMap(bc.immediate.callBuiltinFixedArgs.ast, true);
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


// Char to int
static size_t charToInt(const char* p, size_t & hast) {
    for (size_t i = 0; i < strlen(p); ++i) {
        hast = ((hast << 5) + hast) + p[i];
    }
    return hast;
}

// Hash function, inspired by the way GNUR calculated hashes.
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

// Returns the hast, given the body and env
SEXP getHast(SEXP body, SEXP env) {
    std::stringstream qHast;
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

void insertVTable(DispatchTable* vtable, SEXP hastSym) {
    GeneralMaps::HAST_VTAB_MAP[hastSym] = vtable->container();
}

void insertClosObj(SEXP clos, SEXP hastSym) {
    GeneralMaps::HAST_CLOS_MAP[hastSym] = clos;
}

void insertToBlacklist(SEXP hastSym) {
    if (GeneralMaps::BL_MAP.find(hastSym) != GeneralMaps::BL_MAP.end()) {
        GeneralMaps::BL_MAP[hastSym] = true;
        // TODO
        // serializerCleanup();
    }
}

bool readyForSerialization(SEXP hastSym) {
    // if the hast already corresponds to other src address and is a different closureObj then
    // there was a collision and we cannot use the function and all functions that depend on it

    if (GeneralMaps::HAST_VTAB_MAP.find(hastSym) != GeneralMaps::HAST_VTAB_MAP.end()) {
        insertToBlacklist(hastSym);
        #if DEBUG_BLACKLIST_ENTRIES == 1
        std::cout << "(BLACK) Hast: " << CHAR(PRINTNAME(hastSym)) << std::endl;
        // TODO
        // printSources(vtable, hast);
        #endif
        return false;
    }
    return true;
}

SEXP getVtableContainer(SEXP hastSym) {
    if (GeneralMaps::HAST_VTAB_MAP.find(hastSym) != GeneralMaps::HAST_VTAB_MAP.end()) {
        return GeneralMaps::HAST_VTAB_MAP[hastSym];
    }

    return R_NilValue;
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

unsigned getSrcPoolIndexAtOffset(SEXP hastSym, int requiredOffset) {

    SEXP vtabContainer = getVtableContainer(hastSym);

    if (vtabContainer == R_NilValue) {
        Rf_error("getSrcPoolIndexAtOffset failed!");
    }
    if (!DispatchTable::check(vtabContainer)) {
        Rf_error("getSrcPoolIndexAtOffset vtable corrupted");
    }
    auto vtab = DispatchTable::unpack(vtabContainer);
    auto r = getResultAtOffset(vtab, requiredOffset);

    return r.srcIdx;
}

static SEXP getVtableContainerAtOffset(SEXP hastSym, int requiredOffset) {
    SEXP vtabContainer = getVtableContainer(hastSym);
    if (!vtabContainer) {
        Rf_error("getVtableContainerAtOffset failed!");
    }
    if (!DispatchTable::check(vtabContainer)) {
        Rf_error("getVtableContainerAtOffset vtable corrupted");
    }
    auto vtab = DispatchTable::unpack(vtabContainer);
    auto r = getResultAtOffset(vtab, requiredOffset);
    return r.vtable->container();
}

bool isHastBlacklisted(SEXP hastSym) {
    return GeneralMaps::BL_MAP.find(hastSym) != GeneralMaps::BL_MAP.end();
}

SEXP getVtableContainer(SEXP hastSym, int requiredOffset) {
    return getVtableContainerAtOffset(hastSym, requiredOffset);
}

SEXP getClosContainer(SEXP hastSym) {
    if (GeneralMaps::HAST_CLOS_MAP.find(hastSym) != GeneralMaps::HAST_CLOS_MAP.end()) {
        return GeneralMaps::HAST_CLOS_MAP[hastSym];
    }

    return R_NilValue;
}

}
