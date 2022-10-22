#ifndef RIR_BCLINK_H
#define RIR_BCLINK_H

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


#define GROWTHRATE 5

namespace rir {

    class BitcodeLinkUtil {
    public:
        static bool contextualCompilationSkip;
        static size_t linkTime;
        static SEXP getHast(SEXP body, SEXP env);
        static void populateHastSrcData(DispatchTable* vtable, SEXP hastSym);
        static void insertVTable(DispatchTable* vtable, SEXP hastSym);
        static void insertClosObj(SEXP clos, SEXP hastSym);
        static bool readyForSerialization(SEXP clos, DispatchTable* vtable, SEXP hastSym);

        static void tryUnlocking(SEXP currHastSym);
        static void tryUnlockingOpt(SEXP currHastSym, const unsigned long & con, const int & nargs);
        static void applyMask(DispatchTable * vtab, SEXP hSym);
        static void tryLinking(DispatchTable * vtab, SEXP hSym);
        static void markStale(SEXP currHastSym, const unsigned long & con);
        static Code * getCodeObjectAtOffset(SEXP hastSym, int offset);
        static unsigned getSrcPoolIndexAtOffset(SEXP hastSym, int offset);
        static unsigned getSrcPoolIndexAtOffsetWEAK(SEXP hastSym, int offset);
        static SEXP getVtableContainerAtOffset(SEXP hastSym, int offset);
        static DispatchTable * getVtableAtOffset(DispatchTable * vtab, int offset);
        static void printSources(DispatchTable* vtable, SEXP hastSym);
        static void populateTypeFeedbackData(SEXP container, DispatchTable * vtab);
        static void getTypeFeedbackPtrsAtIndices(std::vector<int> & indices, std::vector<ObservedValues*> & res, DispatchTable * vtab);
        static std::unordered_map<SEXP, SEXP> sourcePoolInverseMapping;
    };

}

#endif
