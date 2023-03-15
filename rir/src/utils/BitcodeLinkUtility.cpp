#include "utils/BitcodeLinkUtility.h"

#include "R/Preserve.h"
#include "R/Protect.h"
#include "R/r.h"
#include "runtime/DispatchTable.h"
#include "utils/FunctionWriter.h"
#include "utils/deserializerRuntime.h"

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
#include "utils/DeserializerDebug.h"

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

static void processWorklistElements(std::vector<BC::PoolIdx> & wlElementVec, size_t & linkTime) {
    std::vector<unsigned> toRemove;
    for (unsigned i = 0; i < wlElementVec.size(); i++) {
        BC::PoolIdx ueIdx = wlElementVec[i];
        SEXP uEleContainer = Pool::get(ueIdx);

        // Processed indices can be removed
        toRemove.push_back(i);

        unsigned * counter = deserializedMetadata::getCounter(uEleContainer);
        *counter = *counter - 1;

        // deserializedMetadata::print(uEleContainer, std::cout, 4);

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
        assert(wl.size() == 0);
        Worklist1::remove(currHastSym);
    }
}

//
// Uses to unlocking element to deserialize and populate dispatch table
//
static void doUnlockingElement(SEXP uEleContainer, size_t & linkTime) {
    auto start = high_resolution_clock::now();

    // DeserializerDebug::infoMessage("Deserializing binary", 4);
    // deserializedMetadata::print(uEleContainer, std::cout, 6);

    rir::pir::PirJitLLVM jit("deserializer");
    jit.deserializeAndPopulateBitcode(uEleContainer);
    jit.finalize();

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    linkTime += duration.count();
}

static std::vector<SEXP> getActiveDependencies(SEXP rMap) {
    std::vector<SEXP> unsatisfiedDeps;
    for (int i = 0; i < Rf_length(rMap); i++) {
        SEXP dep = VECTOR_ELT(rMap, i);
        if (Hast::hastMap.count(dep) == 0) {
            unsatisfiedDeps.push_back(dep);
        }
    }
    return unsatisfiedDeps;
}

void BitcodeLinkUtil::tryLinking(DispatchTable * vtab, SEXP hSym) {
    SEXP binaries = GeneralWorklist::get(hSym);
    //
    // Add hast to the dispatch table as we know this is the 0th offset by default.
    //
    vtab->hast = hSym;
    vtab->offsetIdx = 0;

    if (!binaries) {
        tryUnlocking(hSym);
        GeneralWorklist::remove(hSym);
        return;
    }

    std::vector<BC::PoolIdx> earlyLinkingIdx;

    DeserializerDebug::infoMessage("Deserializer Starting", 0);
    DeserializerDebug::infoMessage(CHAR(PRINTNAME(hSym)), 2);

    for (int i = 0; i < Rf_length(binaries); i++) {
      SEXP binary = VECTOR_ELT(binaries, i);
      if (DeserializerDebug::level > 1) {
        deserializerBinary::print(binary, std::cout, 4);
      }
      int offsetIdx = deserializerBinary::getOffset(binary);
      SEXP dependencies = deserializerBinary::getDependencies(binary);
      auto unsatisfiedDeps = getActiveDependencies(dependencies);
      // Reinterpret the existing binary as an unlocking element i.e.
      DispatchTable * requiredVtab = Hast::getVtableObjectAtOffset(vtab, offsetIdx);
      requiredVtab->hast = hSym;
      requiredVtab->offsetIdx = offsetIdx;
      deserializedMetadata::addVTab(binary, requiredVtab->container());
      deserializedMetadata::addCounter(binary, unsatisfiedDeps.size());
      auto ueIdx = Pool::makeSpace();
      Pool::patch(ueIdx, binary);
      if (unsatisfiedDeps.size() == 0) {
        earlyLinkingIdx.push_back(ueIdx);
        continue;
      }
      for (auto & dep : unsatisfiedDeps) {
        Worklist1::worklist[dep].push_back(ueIdx);
      }
    }
    DeserializerDebug::infoMessage("TryUnlocking", 2);
    tryUnlocking(hSym);

    // Early linking is now semi early linking
    for (auto & ueIdx : earlyLinkingIdx) {
        if (DeserializerDebug::level > 1) {
            DeserializerDebug::infoMessage("Early linking", 2);
        }
        doUnlockingElement(Pool::get(ueIdx), linkTime);
        Pool::patch(ueIdx, R_NilValue);
    }

    GeneralWorklist::remove(hSym);
}
}
