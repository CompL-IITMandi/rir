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


namespace rir {

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
            switch (bc.bc) {
                case Opcode::record_type_:

                    // std::cout << "record_type_(" << i << "): ";
                    // bc.immediate.typeFeedback.print(std::cout);
                    // std::cout << std::endl;
                    // std::cout << "[[ " << &bc.immediate.typeFeedback << " ]]" << std::endl;
                    contextData::addObservedValueToVector(container, &bc.immediate.typeFeedback);

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

void BitcodeLinkUtil::getTypeFeedbackPtrsAtIndices(std::vector<int> & indices, std::vector<ObservedValues*> & res, DispatchTable * vtab) {
    // Indices must be sorted for this to work
    DispatchTable * currVtab = vtab;

    int i = 0;

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
                    // std::cout << "record_type_(" << i << "): ";
                    // feedback->print(std::cout);
                    // std::cout << std::endl;
                    // std::cout << "[[ " << feedback << " ]]" << std::endl;
                    if (std::count(indices.begin(), indices.end(), i)) {
                        res.push_back(feedback);
                    }
                    i++;
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
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callFixedArgs.ast << std::endl;
                    #endif
                    addSrcToMap(bc.immediate.callFixedArgs.ast, false);
                    indexOffset++;
                    break;
                case Opcode::call_dots_:
                    #if PRINT_HAST_SRC_ENTRIES == 1
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callFixedArgs.ast << std::endl;
                    #endif
                    addSrcToMap(bc.immediate.callFixedArgs.ast, false);
                    indexOffset++;
                    break;
                case Opcode::call_builtin_:
                    #if PRINT_HAST_SRC_ENTRIES == 1
                    std::cout << "src_hast_entry[constant_pool] at indexOffset: " << indexOffset << "," << bc.immediate.callBuiltinFixedArgs.ast << std::endl;
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
    for (unsigned int i = 0; i < wlElementVec.size(); i++) {



        BC::PoolIdx ueIdx = wlElementVec[i];
        SEXP uEleContainer = Pool::get(ueIdx);

        // generalUtil::printSpace(2);
        // std::cout << "processWorklistElements" << std::endl;
        // UnlockingElement::print(uEleContainer, 4);
        //
        // If this was an optimistic unlock, then make sure that the
        // (numargs of available >= numargs of expected)
        // If not, we cannot remove the worklist element
        //
        if (nargsPassed) {
            if (unsigned * numArgs = UnlockingElement::getNumArgs(uEleContainer)) {
                if (nargs < *numArgs) {
                    //
                    // Optimistic unlock worst case fail
                    // ?TODO: If this case happens, find out why it does
                    //
                    std::cerr << "[TO DEBUG] Optimistic unlock worst case fail" << std::endl;
                    continue;
                }
            }
        }

        // Processed indices can be removed
        toRemove.push_back(i);

        int * counter = UnlockingElement::getCounter(uEleContainer);
        *counter = *counter - 1;

        if (*counter == 0) {
            //
            // Do unlocking if counter becomes 0
            //
            doUnlockingElement(uEleContainer, linkTime);
            UnlockingElement::remove(ueIdx);
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
    if (Worklist1::worklist.count(currHastSym) > 0) {
        std::vector<BC::PoolIdx> & wl = Worklist1::worklist[currHastSym];
        processWorklistElements(wl, linkTime);

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
    SEXP ddCont = GeneralWorklist::get(hSym);
    //
    // Add hast to the dispatch table as we know this is the 0th offset by default.
    //

    SEXP hastSym  = deserializerData::getHast(ddCont);
    vtab->hast = hastSym;

    deserializerData::iterateOverUnits(ddCont, [&](SEXP ddContainer, SEXP offsetUnitContainer, SEXP contextUnitContainer, SEXP binaryUnitContainer) {
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

        // 2. Versioning
        int versioning = contextUnit::getVersioningAsInt(contextUnitContainer);

        // 3. Counter Value
        SEXP rMap = binaryUnit::getReqMap(binaryUnitContainer);
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
            // Do linking here using the unlock element
            //

            doUnlockingElement(Pool::get(ueIdx), linkTime);

            Pool::patch(ueIdx, R_NilValue);
            return;
        }

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

                        UnlockingElement::addNumArgs(Pool::get(ueIdx), std::stoi(nargs));
                    }

                    //
                    // Add to worklist2
                    //

                    Worklist2::worklist[hastKey].push_back(ueIdx);
                } else {
                    //
                    // Add to worklist1
                    //

                    Worklist1::worklist[dep].push_back(ueIdx);
                }
            }
        }
    });

    // Remove the generalWorklistElement
    GeneralWorklist::remove(hSym);
}

size_t BitcodeLinkUtil::linkTime = 0;
bool BitcodeLinkUtil::contextualCompilationSkip = getenv("SKIP_CONTEXTUAL_COMPILATION") ? true : false;
}
