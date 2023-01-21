#pragma once

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

namespace rir {

    class BitcodeLinkUtil {
    public:
        static size_t deoptCount;
        static size_t linkTime;
        static size_t llvmLoweringTime;
        static size_t llvmSymbolsTime;

        static void tryLinking(DispatchTable * vtab, SEXP hSym);
        static void tryUnlocking(SEXP currHastSym);
    };

}
