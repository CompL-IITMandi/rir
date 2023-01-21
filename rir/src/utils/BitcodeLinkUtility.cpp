#include "utils/BitcodeLinkUtility.h"

#include "R/Preserve.h"
#include "R/Protect.h"
#include "R/r.h"
#include "runtime/DispatchTable.h"
#include "utils/FunctionWriter.h"
#include "utils/Pool.h"
// #include "utils/UMap.h"

#include <unordered_map>
#include <iostream>
#include <functional>
#include <cassert>

// #include "runtimePatches.h"
#include "R/Printing.h"
#include "api.h"

#include "compiler/pir/module.h"
// #include "compiler/log/stream_logger.h"
#include "compiler/compiler.h"
#include "compiler/backend.h"

#include "utils/WorklistManager.h"
#include "utils/deserializerData.h"

#include "utils/Hast.h"

#include "compiler/native/pir_jit_llvm.h"

#include <chrono>
using namespace std::chrono;


#define GROWTHRATE 5
#define PRINT_LINKING_STATUS 0
#define PRINT_WORKLIST_ENTRIES 0
#define DEBUG_BLACKLIST 0
#define DEBUG_DESERIALIZER_CHECKPOINTS 0


namespace rir {
size_t BitcodeLinkUtil::linkTime = 0;
size_t BitcodeLinkUtil::deoptCount = 0;
size_t BitcodeLinkUtil::llvmLoweringTime = 0;
size_t BitcodeLinkUtil::llvmSymbolsTime = 0;


static void doUnlockingElement(SEXP uEleContainer, size_t & linkTime);

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
// Uses to unlocking element to deserialize and populate dispatch table
//
static void doUnlockingElement(SEXP uEleContainer, size_t & linkTime) {
    #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
    std::cerr << "(c) DESERIALIZING: " << UnlockingElement::getPathPrefix(uEleContainer) << std::endl;
    #endif
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

    rir::pir::PirJitLLVM jit("deserializer");
    jit.deserializeAndPopulateBitcode(uEleContainer);
    jit.finalize();

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    linkTime += duration.count();
    #if DEBUG_DESERIALIZER_CHECKPOINTS == 1
    std::cerr << "(c) DESERIALIZING DONE " << UnlockingElement::getPathPrefix(uEleContainer) << std::endl;
    #endif
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


    // REnvHandler hastVtabMap(HAST_VTAB_MAP);
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
            if (Hast::hastMap.count(hastOfReq) > 0) {
                // If the dependency already exists, we make the slot null
                SET_VECTOR_ELT(rMap, i, R_NilValue);
            } else {
                unsatisfiedDependencies++;
            }
        } else {
            // check if optimistic site already exists
            if (Hast::hastMap.count(hastOfReq) > 0) {
                SEXP vtabContainer = Hast::hastMap[hastOfReq].vtabContainer;
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
        DispatchTable * requiredVtab = Hast::getVtableObjectAtOffset(vtab, offsetIdx);

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

            UnlockingElement::addGTFSlotInfo(
                Pool::get(ueIdx),
                contextUnit::getFBSlots(contextUnitContainer));
            UnlockingElement::addGFunTFInfo(
                Pool::get(ueIdx), binaryUnit::getFBData(binaryUnitContainer));
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

    // DES-TODO
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
}
