#pragma once

#include "R/r.h"
#include <unordered_map>
#include "runtime/DispatchTable.h"

namespace rir {

struct HastData {
    SEXP vtabContainer;
    SEXP clos;
};

struct HastInfo {
    SEXP hast;
    unsigned offsetIndex;
};

class Hast {
    public:

    static std::unordered_map<SEXP, HastData> hastMap;
    static std::unordered_map<unsigned, HastInfo> sPoolHastMap;
    static std::unordered_map<unsigned, HastInfo> cPoolHastMap;

    static void populateHastSrcData(DispatchTable* vtable, SEXP hastSym);

    static bool isAnonEnv(SEXP env);
    static SEXP getHast(SEXP body, SEXP env);
};
}
