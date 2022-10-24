#include "BitcodeLinkUtility.h"

#include "R/Preserve.h"
#include "R/Protect.h"
#include "R/r.h"
#include "runtime/DispatchTable.h"
#include "utils/FunctionWriter.h"
#include "utils/Pool.h"
#include "utils/UMap.h"

#include <unordered_map>
#include <iostream>
#include <functional>
#include <cassert>

#include "runtimePatches.h"
#include "R/Printing.h"
#include "api.h"

#include "compiler/pir/module.h"
#include "compiler/log/stream_logger.h"
#include "compiler/compiler.h"
#include "compiler/backend.h"

#include "utils/WorklistManager.h"
#include "utils/deserializerData.h"

#include <chrono>
using namespace std::chrono;


#define GROWTHRATE 5
#define PRINT_LINKING_STATUS 0
#define PRINT_WORKLIST_ENTRIES 0
#define DEBUG_BLACKLIST 0
#define PRINT_HAST_SRC_ENTRIES 0
#define DEBUG_DESERIALIZER_CHECKPOINTS 0


namespace rir {

std::unordered_map<SEXP, SEXP> BitcodeLinkUtil::sourcePoolInverseMapping;
//
// AST Hashing function
//

static size_t charToInt(const char* p, size_t & hast) {
    for (size_t i = 0; i < strlen(p); ++i) {
        hast = ((hast << 5) + hast) + p[i];
    }
    return hast;
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

SEXP BitcodeLinkUtil::getHast(SEXP body, SEXP env) {
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

void BitcodeLinkUtil::populateTypeFeedbackData(SEXP container, DispatchTable * vtab) {
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

void BitcodeLinkUtil::getTypeFeedbackPtrsAtIndices(std::vector<int> & indices, std::vector<ObservedValues*> & res, DispatchTable * vtab) {
    // Indices must be sorted for this to work
    DispatchTable * currVtab = vtab;

    int reqIdx = 0;

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
            switch (bc.bc) {
                case Opcode::record_type_: {
                    // switch (*pos) {
                    // case Opcode::record_type_: {
                    // assert(*pos == Opcode::record_type_);
                    if (std::count(indices.begin(), indices.end(), reqIdx)) {
                        // std::cout << "FOR CODE:  " << c << std::endl;
                        ObservedValues* feedback = (ObservedValues*)(pc + 1);
                        // std::cout << "[I]PERCIEVED AT: " << feedback << std::endl;
                        // feedback->print(std::cout);
                        // std::cout << std::endl;

                        // std::cout << "[I]ACTUAL: " << std::endl;
                        // bc.immediate.typeFeedback.print(std::cout);
                        // std::cout << std::endl;
                        res.push_back(feedback);
                    }
                    reqIdx++;
                }
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
    Function * genesisFunObj = genesisCodeObj->function();

    iterateOverCodeObjs(genesisCodeObj, genesisFunObj);
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

Code * BitcodeLinkUtil::getCodeObjectAtOffset(SEXP hastSym, int requiredOffset) {
    REnvHandler vtabMap(HAST_VTAB_MAP);
    SEXP vtabContainer = vtabMap.get(hastSym);
    if (!vtabContainer) {
        Rf_error("getCodeObjectAtOffset failed!");
    }
    if (!DispatchTable::check(vtabContainer)) {
        Rf_error("getCodeObjectAtOffset vtable corrupted");
    }
    auto vtab = DispatchTable::unpack(vtabContainer);
    auto r = getResultAtOffset(vtab, requiredOffset);

    return r.code;
}

unsigned BitcodeLinkUtil::getSrcPoolIndexAtOffset(SEXP hastSym, int requiredOffset) {
    REnvHandler vtabMap(HAST_VTAB_MAP);
    SEXP vtabContainer = vtabMap.get(hastSym);
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

unsigned BitcodeLinkUtil::getSrcPoolIndexAtOffsetWEAK(SEXP hastSym, int requiredOffset) {
    REnvHandler vtabMap(HAST_VTAB_MAP);
    SEXP vtabContainer = vtabMap.get(hastSym);
    if (!vtabContainer) {
        return 0;
    }
    if (!DispatchTable::check(vtabContainer)) {
        return 0;
    }
    auto vtab = DispatchTable::unpack(vtabContainer);
    auto r = getResultAtOffset(vtab, requiredOffset);

    return r.srcIdx;
}

SEXP BitcodeLinkUtil::getVtableContainerAtOffset(SEXP hastSym, int requiredOffset) {
    REnvHandler vtabMap(HAST_VTAB_MAP);
    SEXP vtabContainer = vtabMap.get(hastSym);
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

DispatchTable * BitcodeLinkUtil::getVtableAtOffset(DispatchTable * vtab, int requiredOffset) {
    auto res = getResultAtOffset(vtab, requiredOffset);
    return res.vtable;
}

void BitcodeLinkUtil::populateHastSrcData(DispatchTable* vtable, SEXP parentHast) {
    REnvHandler srcHastMap(SRC_HAST_MAP);
    DispatchTable * currVtab = vtable;
    unsigned indexOffset = 0;

    auto addSrcToMap = [&] (const unsigned & src, bool sourcePool = true) {
        // create a mapping for each src [representing code object to its hast and offset index]
        SEXP srcSym;
        if (sourcePool) {
            srcSym = Rf_install(std::to_string(src).c_str());
        } else {
            srcSym = Rf_install((std::to_string(src) + "_cp").c_str());
        }
        SEXP indexSym = Rf_install(std::to_string(indexOffset).c_str());

        if (!srcHastMap.get(srcSym)) {
            SEXP resVec;
            PROTECT(resVec = Rf_allocVector(VECSXP, 2));
            SET_VECTOR_ELT(resVec, 0, parentHast);
            SET_VECTOR_ELT(resVec, 1, indexSym);
            srcHastMap.set(srcSym, resVec);
            UNPROTECT(1);
        }

        if (!sourcePool) {
            if (sourcePoolInverseMapping.count(Pool::get(src)) == 0) {
                sourcePoolInverseMapping[Pool::get(src)] = srcHastMap.get(srcSym);
            }
        }
        // else {
        //     std::cout << "More than one entry for the same src" << std::endl;
        // }

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
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callFixedArgs.ast << " [ " << Pool::get(bc.immediate.callFixedArgs.ast) << "]" << std::endl;
                    #endif
                    addSrcToMap(bc.immediate.callFixedArgs.ast, false);
                    indexOffset++;
                    break;
                case Opcode::call_dots_:
                    #if PRINT_HAST_SRC_ENTRIES == 1
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callFixedArgs.ast << " [ " << Pool::get(bc.immediate.callFixedArgs.ast) << "]" << std::endl;
                    #endif
                    addSrcToMap(bc.immediate.callFixedArgs.ast, false);
                    indexOffset++;
                    break;
                case Opcode::call_builtin_:
                    #if PRINT_HAST_SRC_ENTRIES == 1
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callBuiltinFixedArgs.ast << " [ " << Pool::get(bc.immediate.callFixedArgs.ast) << "]" << std::endl;
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

void BitcodeLinkUtil::printSources(DispatchTable* vtable, SEXP parentHast) {
    // TODO
}

void BitcodeLinkUtil::insertVTable(DispatchTable* vtable, SEXP hastSym) {
    REnvHandler vtabMap(HAST_VTAB_MAP);
    vtabMap.set(hastSym, vtable->container());
}

void BitcodeLinkUtil::insertClosObj(SEXP clos, SEXP hastSym) {
    REnvHandler closMap(HAST_CLOS_MAP);
    closMap.set(hastSym, clos);
}

static void insertToBlacklist(SEXP hastSym) {
    REnvHandler blacklistMap(BL_MAP);
    if (!blacklistMap.get(hastSym)) {
        blacklistMap.set(hastSym, R_TrueValue);
        #if DEBUG_BLACKLIST == 1
        std::cout << "(R) Blacklisting: " << CHAR(PRINTNAME(hastSym)) << std::endl;
        #endif
        serializerCleanup();
    }
}

bool BitcodeLinkUtil::readyForSerialization(SEXP clos, DispatchTable* vtable, SEXP hastSym) {
    // if the hast already corresponds to other src address and is a different closureObj then
    // there was a collision and we cannot use the function and all functions that depend on it
    REnvHandler vtabMap(HAST_VTAB_MAP);
    if (vtabMap.get(hastSym)) {
        insertToBlacklist(hastSym);
        return false;
    }
    return true;
}

static void doUnlockingElement(SEXP uEleContainer, size_t & linkTime) {
    auto start = high_resolution_clock::now();

    // // if this context was already added due to unexpected compilation at runtime, skip addition, this can result in duplicate LLVM symbols
    // for (size_t i = 0; i < vtab->size(); i++) {
    //     auto entry = vtab->get(i);
    //     if (entry->context().toI() == contextData::getContext(cData)) {
    //         #if PRINT_LINKING_STATUS == 1
    //         std::cout << "duplicate linkage: " << CHAR(PRINTNAME(hSym)) << "_" << CHAR(PRINTNAME(offsetSymbol)) << "_" << contextData::getContext(cData) << std::endl;
    //         #endif
    //         return;
    //     }
    // }

    // UnlockingElement::print(uEleContainer, 0);

    std::string name(UnlockingElement::getPathPrefix(uEleContainer));

    pir::StreamLogger logger(pir::DebugOptions::DefaultDebugOptions);
    logger.title("Deserializing " + name);

    pir::Module* m = new pir::Module;

    pir::Compiler cmp(m, logger);
    pir::Backend backend(m, logger, name);

    backend.deserializeAndPopulateBitcode(uEleContainer);
    delete m;


    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    linkTime += duration.count();

    logger.title("Deserialized " + name);
}

static void processWorklistElements(std::vector<BC::PoolIdx> & wlElementVec, size_t & linkTime, bool nargsPassed = false, const unsigned & nargs = 0) {
    std::vector<unsigned int> toRemove;

    if (nargsPassed) {
        #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
        std::cerr << "(c) Working on optimistic worklist!" << std::endl;
        #endif
        for (unsigned int i = 0; i < wlElementVec.size(); i++) {
            BC::PoolIdx ueIdx = wlElementVec[i];
            SEXP optuEleContainer = Pool::get(ueIdx);

            #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
            std::cerr << "(c) Reducing counter for OUE" << ueIdx << std::endl;
            #endif

            #if PRINT_WORKLIST_ENTRIES == 1
            OptUnlockingElement::print(optuEleContainer, 2);
            #endif

            // generalUtil::printSpace(2);
            // std::cout << "processWorklistElements" << std::endl;
            // UnlockingElement::print(uEleContainer, 4);
            //
            // If this was an optimistic unlock, then make sure that the
            // (numargs of available >= numargs of expected)
            // If not, we cannot remove the worklist element
            //
            if (unsigned * numArgs = OptUnlockingElement::getNumArgs(optuEleContainer)) {
                if (!(nargs >= (*numArgs))) {
                    // std::cout << "OptUnlockingElement: " << optuEleContainer << std::endl;
                    // OptUnlockingElement::print(optuEleContainer, 0);
                    //
                    // Optimistic unlock worst case fail
                    // ?TODO: If this case happens, find out why it does
                    //
                    std::cerr << "[TO DEBUG] Optimistic unlock worst case fail, nargs: " << nargs << ", expectedNargs: " << *numArgs << std::endl;
                    // std::cerr << "[WARNING] Ignoring check, lets see when this breaks!" << std::endl;

                    continue;
                }
            }
            // Processed indices can be removed
            toRemove.push_back(i);

            SEXP uEleContainer = OptUnlockingElement::getUE(optuEleContainer);

            int * counter = UnlockingElement::getCounter(uEleContainer);
            *counter = *counter - 1;

            if (*counter == 0) {
                #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
                std::cerr << "(c) Counter is zero, now performing linking!" << std::endl;
                #endif
                //
                // Do unlocking if counter becomes 0
                //
                doUnlockingElement(uEleContainer, linkTime);
                UnlockingElement::remove(ueIdx);
            }
        }
    } else {
        #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
        std::cerr << "(c) Working on simple worklist!" << std::endl;
        #endif
        for (unsigned int i = 0; i < wlElementVec.size(); i++) {
            BC::PoolIdx ueIdx = wlElementVec[i];
            SEXP uEleContainer = Pool::get(ueIdx);

            #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
            std::cerr << "(c) Reducing counter for UE" << ueIdx << std::endl;
            #endif

            #if PRINT_WORKLIST_ENTRIES == 1
            UnlockingElement::print(ueIdx, 2);
            #endif


            // Processed indices can be removed
            toRemove.push_back(i);

            int * counter = UnlockingElement::getCounter(uEleContainer);
            *counter = *counter - 1;

            if (*counter == 0) {
                #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
                std::cerr << "(c) Counter is zero, now performing linking!" << std::endl;
                #endif
                //
                // Do unlocking if counter becomes 0
                //
                doUnlockingElement(uEleContainer, linkTime);
                UnlockingElement::remove(ueIdx);
            }
        }
    }


    // https://stackoverflow.com/questions/35055227/how-to-erase-multiple-elements-from-stdvector-by-index-using-erase-function
    // Sorting toRemove is ascending order is needed but
    // due to the order of creation they are already sorted
    // std::sort(toRemove.begin(), toRemove.end());

    //
    // Traverse the remove list in decreasing order,
    // When we remove those indices from the wlElementVec
    // the relative order of indices to remove does not change
    //

    for (std::vector<unsigned int>::reverse_iterator i = toRemove.rbegin(); i != toRemove.rend(); ++ i) {
        wlElementVec.erase(wlElementVec.begin() + *i);
    }
}

//
// Check if Worklist1 has work for the current hast
//
void BitcodeLinkUtil::tryUnlocking(SEXP currHastSym) {

    #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
    std::cerr << "(c) Unlocking worklist-1 for [" << CHAR(PRINTNAME(currHastSym)) << "]" << std::endl;
    #endif

    // std::cout << "Worklist1[" << CHAR(PRINTNAME(currHastSym)) << "] query" << std::endl;
    // std::cout << "Worklist1 Bindings" << std::endl;
    // for (auto & ele : Worklist1::worklist) {
    //     std::cout << " " << CHAR(PRINTNAME(ele.first)) << std::endl;
    // }

    #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
    std::cerr << "(c) " << Worklist1::worklist.count(currHastSym) << " worklist-1 entries!" << std::endl;
    #endif

    if (Worklist1::worklist.count(currHastSym) > 0) {
        std::vector<BC::PoolIdx> & wl = Worklist1::worklist[currHastSym];

        #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
        std::cerr << "(c) Started processing worklist, size: " << wl.size() << std::endl;
        #endif

        processWorklistElements(wl, linkTime);

        #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
        std::cerr << "(c) Finished processing worklist, size: " << wl.size() << std::endl;
        #endif


        if (wl.size() == 0) {
            Worklist1::remove(currHastSym);
        }
    }
}

//
// Check if Worklist2 has work for the current hast_context
//
void BitcodeLinkUtil::tryUnlockingOpt(SEXP currHastSym, const unsigned long & con, const int & nargs) {
    std::stringstream ss;
    ss << CHAR(PRINTNAME(currHastSym)) << "_" << con;
    SEXP worklistKey = Rf_install(ss.str().c_str());

    // std::cout << "Worklist2[" << CHAR(PRINTNAME(worklistKey)) << "] query" << std::endl;
    // std::cout << "Worklist2 Bindings" << std::endl;
    // for (auto & ele : Worklist2::worklist) {
    //     std::cout << " " << CHAR(PRINTNAME(ele.first)) << std::endl;
    // }

    if (Worklist2::worklist.count(worklistKey) > 0) {
        std::vector<BC::PoolIdx> & wl = Worklist2::worklist[worklistKey];
        processWorklistElements(wl, linkTime, true, nargs);

        if (wl.size() == 0) {
            Worklist2::remove(worklistKey);
        }
    }
}

void BitcodeLinkUtil::markStale(SEXP currHastSym, const unsigned long & con) {
    // std::stringstream ss;
    // ss << CHAR(PRINTNAME(currHastSym)) << "_" << con;
    // SEXP optMapKey = Rf_install(ss.str().c_str());

    // REnvHandler linkageMap(LINKAGE_MAP);

    // if (linkageMap.get(optMapKey)) {
    //     #if PRINT_LINKING_STATUS == 1
    //     std::cout << "  MARKED STALE: " << ss.str() << std::endl;
    //     #endif
    //     linkageMap.remove(optMapKey);
    // }
}

void BitcodeLinkUtil::applyMask(DispatchTable * vtab, SEXP hSym) {

}

//
// Removes already satisfied dependencies and returns number of unsatisfied ones
//
static std::pair<int, std::vector<int>> reduceReqMap(SEXP rMap) {
    int unsatisfiedDependencies = 0;
    std::vector<int> optimisticIdx;

    // // Original Requirement Map
    // std::cout << "ORIGINAL REQ_MAP: (" << Rf_length(rMap) << "): [ ";
    // for (int i = 0; i < Rf_length(rMap); i++) {
    //     std::cout << CHAR(PRINTNAME(VECTOR_ELT(rMap, i))) << " ";
    // }
    // std::cout << "]" << std::endl;


    REnvHandler hastVtabMap(HAST_VTAB_MAP);
    for (int i = 0; i < Rf_length(rMap); i++) {
        SEXP dep = VECTOR_ELT(rMap, i);

        SEXP hastOfReq;
        unsigned long con;
        int numArgs;

        bool optimisticCase = false;
        std::string n(CHAR(PRINTNAME(dep)));

        auto firstDel = n.find('_');
        if (firstDel != std::string::npos) {
            auto n = std::string(CHAR(PRINTNAME(dep)));
            auto firstDel = n.find('_');
            auto secondDel = n.find('_', firstDel + 1);
            auto hast = n.substr(0, firstDel);
            auto context = n.substr(firstDel + 1, secondDel - firstDel - 1);
            auto nargs = n.substr(secondDel + 1);

            hastOfReq = Rf_install(hast.c_str());
            con = std::stoul(context);
            numArgs = std::stoi(nargs);

            optimisticIdx.push_back(i);
            optimisticCase = true;
        } else {
            hastOfReq = dep;
        }

        if (!optimisticCase) {
            if (!hastVtabMap.get(hastOfReq)) {
                unsatisfiedDependencies++;
            } else {
                // If the dependency already exists, we make the slot null
                SET_VECTOR_ELT(rMap, i, R_NilValue);
            }
        } else {
            // check if optimistic site already exists
            if (SEXP vtabContainer = hastVtabMap.get(hastOfReq)) {
                if (!DispatchTable::check(vtabContainer)) {
                    Rf_error("linking error, corrupted vtable");
                }

                bool optimisticSiteExists = false;
                DispatchTable * requiredVtab = DispatchTable::unpack(vtabContainer);
                for (size_t i = 0; i < requiredVtab->size(); i++) {
                    auto entry = requiredVtab->get(i);
                    if (entry->context().toI() == con &&
                        entry->signature().numArguments >= (unsigned)numArgs) {
                            optimisticSiteExists = true;
                            break;
                    }
                }

                if (!optimisticSiteExists) {
                    unsatisfiedDependencies++;
                } else {
                    // If the dependency already exists, we make the slot null
                    SET_VECTOR_ELT(rMap, i, R_NilValue);
                }

            } else {
                unsatisfiedDependencies++;
            }
        }


    }

    // // Reduced Requirement Map
    // std::cout << "REDUCED REQ_MAP: (" << Rf_length(rMap) << "): [ ";
    // for (int i = 0; i < Rf_length(rMap); i++) {
    //     if (VECTOR_ELT(rMap, i) != R_NilValue)
    //     std::cout << CHAR(PRINTNAME(VECTOR_ELT(rMap, i))) << " ";
    // }
    // std::cout << "]" << std::endl;

    return std::pair<int, std::vector<int>>(unsatisfiedDependencies, optimisticIdx);
}

void BitcodeLinkUtil::tryLinking(DispatchTable * vtab, SEXP hSym) {
    //
    // Level 0 - Only call context dispatch
    // Level 1 - Context + multi binary dispatch
    // Level 2 - Context + multi binary dispatch + Type Versioning
    //
    static int L2LEVEL = getenv("L2_LEVEL") ? std::stoi(getenv("L2_LEVEL")) : 10;

    static bool ONLY_SIMPLE_BINS = getenv("ONLY_SIMPLE_BINS") ? getenv("ONLY_SIMPLE_BINS")[0] = '1' : false;

    static bool ONLY_REV = getenv("ONLY_REV") ? getenv("ONLY_REV")[0] = '1' : false;

    //
    // Only applies the mask to the dispatch table
    //
    static bool ONLYMASK = getenv("ONLY_APPLY_MASK") ? std::stoi(getenv("ONLY_APPLY_MASK")) == 1 : false;

    //
    // Deserialization of binaries + applying the mask
    //
    static bool MASK = getenv("APPLY_MASK") ? std::stoi(getenv("APPLY_MASK")) == 1 : false;

    SEXP ddCont = GeneralWorklist::get(hSym);
    //
    // Add hast to the dispatch table as we know this is the 0th offset by default.
    //

    #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
    std::cerr << "(c) Deserializer Started [" << CHAR(PRINTNAME(hSym)) << "]" << std::endl;
    deserializerData::print(ddCont, 2);
    #endif



    SEXP hastSym  = deserializerData::getHast(ddCont);
    vtab->hast = hastSym;

    // Early linking breaks the implicit binary ordering, should complete worklist1 before linking these binaries
    std::vector<BC::PoolIdx> earlyLinkingIdx;

    deserializerData::iterateOverUnits(ddCont, [&](SEXP ddContainer, SEXP offsetUnitContainer, SEXP contextUnitContainer, SEXP binaryUnitContainer, unsigned int i, unsigned int last) {

        if (ONLY_REV) {
            if (i + 1 != last) {
                return;
            }
        }

        //
        // Creating the UnlockElement
        //

        int offsetIdx = offsetUnit::getOffsetIdxAsInt(offsetUnitContainer);
        SEXP epoch    = binaryUnit::getEpoch(binaryUnitContainer);

        // 0. PathPrefix
        std::stringstream pathPrefix;
        pathPrefix << CHAR(PRINTNAME(hastSym)) << "_" << offsetIdx << "_" << CHAR(PRINTNAME(epoch));

        // 1. Dispatch Table
        DispatchTable * requiredVtab = getVtableAtOffset(vtab, offsetIdx);

        //
        // MASK RELATED
        //

        if (MASK) {
            requiredVtab->mask = rir::Context(offsetUnit::getMaskAsUnsignedLong(offsetUnitContainer));
        }

        if (ONLYMASK) {
            requiredVtab->mask = rir::Context(offsetUnit::getMaskAsUnsignedLong(offsetUnitContainer));
            return;
        }

        // 2. Versioning
        int versioning = contextUnit::getVersioningAsInt(contextUnitContainer);

        //
        // Skip doing anything if we do not want to allow that versioning level.
        //
        if (versioning > L2LEVEL) {
            return;
        }

        // 3. Counter Value
        SEXP rMap = binaryUnit::getReqMap(binaryUnitContainer);

        if (ONLY_SIMPLE_BINS) {
            if (Rf_length(rMap) > 0) {
                return;
            }
        }

        auto reduceRes = reduceReqMap(rMap);

        // 4. nArgs - conditionally added

        // 5. context
        unsigned long con = contextUnit::getContextAsUnsignedLong(contextUnitContainer);

        // 6. TFSlotInfo - added when versioning is 2
        // 7. FunTF      - added when versioning is 2

        auto ueIdx = UnlockingElement::createWorklistElement(
            pathPrefix.str().c_str(),
            requiredVtab->container(),
            versioning,
            reduceRes.first,
            con);

        if (versioning == 2) {
            UnlockingElement::addTFSlotInfo(Pool::get(ueIdx), contextUnit::getTFSlots(contextUnitContainer));
            UnlockingElement::addFunTFInfo(Pool::get(ueIdx), binaryUnit::getTVData(binaryUnitContainer));
        }

        // UnlockingElement::print(wlIdx, 0);

        // Early linking case, no worklist used
        if (reduceRes.first == 0) {
            //
            // Do linking here using the unlock element, this is now delayed
            // to preserve linking order of binaries
            //

            earlyLinkingIdx.push_back(ueIdx);

            return;
        }

        // if (REV_DISPATCH > 0) {
        //     Pool::patch(ueIdx, R_NilValue);
        //     return;
        // }
        std::vector<int> & optIdx = reduceRes.second;

        for (int i = 0; i < Rf_length(rMap); i++) {
            if (VECTOR_ELT(rMap, i) != R_NilValue) {
                SEXP dep = VECTOR_ELT(rMap, i);
                bool optimisticCase = std::find(optIdx.begin(), optIdx.end(), i) != optIdx.end();

                if (optimisticCase) {
                    //
                    // Key: HAST_CONTEXT
                    // When function gets inserted into the vtable, we create this key
                    // using the stored hast and inserted context.
                    // We do work on worklist two if the key exists
                    //
                    SEXP hastKey;

                    BC::PoolIdx optIdx;

                    {
                        //
                        // In optimistic case the DEP_HAST is
                        // HAST_CONTEXT_NARGS
                        //
                        auto n = std::string(CHAR(PRINTNAME(dep)));
                        auto firstDel = n.find('_');
                        auto secondDel = n.find('_', firstDel + 1);
                        auto hast = n.substr(0, firstDel);
                        auto context = n.substr(firstDel + 1, secondDel - firstDel - 1);
                        auto nargs = n.substr(secondDel + 1);

                        hastKey = Rf_install(n.substr(0, secondDel).c_str());

                        // std::cout << "adding nargs to: " << Pool::get(ueIdx) << ", nargs: " << n << std::endl;

                        // UnlockingElement::addNumArgs(Pool::get(ueIdx), std::stoi(nargs));

                        optIdx = OptUnlockingElement::createOptWorklistElement(std::stoi(nargs), Pool::get(ueIdx));
                    }

                    //
                    // Add to worklist2
                    //

                    Worklist2::worklist[hastKey].push_back(optIdx);

                    #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
                    std::cerr << "(c) Adding worklist-2 entry for " << CHAR(PRINTNAME(hastKey)) << " at OUE" << optIdx << std::endl;
                    OptUnlockingElement::print(optIdx, 2);
                    #endif
                } else {
                    //
                    // Add to worklist1
                    //

                    Worklist1::worklist[dep].push_back(ueIdx);

                    #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
                    std::cerr << "(c) Adding worklist-1 entry for " << CHAR(PRINTNAME(dep)) << " at UE" << ueIdx << std::endl;
                    UnlockingElement::print(ueIdx, 2);
                    #endif
                }
            }
        }
    });

    // Do worklist 1
    tryUnlocking(hSym);

    #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
    std::cerr << "(c) " << earlyLinkingIdx.size() << " semi-early linking targets!" << std::endl;
    #endif

    // Early linking is now semi early linking, to preserve implicit binary ordering for V = 1 and V = 2 dispatch
    for (auto & ueIdx : earlyLinkingIdx) {
        #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
        std::cerr << "(c) SEMI EARLY LINKING FOR UE" << ueIdx << std::endl;
        #endif
        doUnlockingElement(Pool::get(ueIdx), linkTime);
        Pool::patch(ueIdx, R_NilValue);
    }

    // Remove the generalWorklistElement
    GeneralWorklist::remove(hSym);
}

void BitcodeLinkUtil::printValidLookupIndices(DispatchTable * vtab) {
    // Indices must be sorted for this to work
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
            switch (bc.bc) {
                case Opcode::record_type_: {
                    // switch (*pos) {
                    // case Opcode::record_type_: {
                    // assert(*pos == Opcode::record_type_);
                    ObservedValues* feedback = (ObservedValues*)(pc + 1);
                    std::cout << "PERCIEVED AT: " << feedback << std::endl;
                    feedback->print(std::cout);
                    std::cout << std::endl;

                    std::cout << "ACTUAL: " << std::endl;
                    bc.immediate.typeFeedback.print(std::cout);
                    std::cout << std::endl;

                }
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
    Function * genesisFunObj = genesisCodeObj->function();

    iterateOverCodeObjs(genesisCodeObj, genesisFunObj);
}


size_t BitcodeLinkUtil::linkTime = 0;
bool BitcodeLinkUtil::contextualCompilationSkip = getenv("SKIP_CONTEXTUAL_COMPILATION") ? true : false;
}
