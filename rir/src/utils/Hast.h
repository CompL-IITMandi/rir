#pragma once

#include "R/r.h"
#include <unordered_map>
#include <set>
#include "runtime/DispatchTable.h"

namespace rir {

struct HastData {
    SEXP vtabContainer;
    SEXP clos;
};

struct HastInfo {
    SEXP hast;
    unsigned offsetIndex;

    unsigned src;

    bool isValid() {
        return hast != R_NilValue;
    }
};

class Hast {
    public:

    static std::set<SEXP> blacklist;
    static std::unordered_map<SEXP, HastData> hastMap;
    static std::unordered_map<unsigned, HastInfo> sPoolHastMap;
    static std::unordered_map<unsigned, HastInfo> cPoolHastMap;

    static std::unordered_map<SEXP, HastInfo> cPoolInverseMap;
    static std::unordered_map<SEXP, HastInfo> sPoolInverseMap;

    static std::unordered_map<int, SEXP> debugMap;
    static int debugIdx;

    static int genDebugIdx() {
        return debugIdx++;
    }

    static HastInfo getHastInfo(const unsigned & srcIdx, const bool & sourcePool) {
        if (sourcePool) {
            if (sPoolHastMap.count(srcIdx) > 0) {
                auto data = sPoolHastMap[srcIdx];
                if (!(blacklist.count(data.hast) > 0)) {
                    return data;
                }
            }
        } else {
            if (cPoolHastMap.count(srcIdx) > 0) {
                auto data = cPoolHastMap[srcIdx];
                if (!(blacklist.count(data.hast) > 0)) {
                    return data;
                }
            }
        }

        return {R_NilValue, 0};
    }


    static void populateHastSrcData(DispatchTable* vtable, SEXP hastSym);
    static void printHastSrcData(DispatchTable* vtable, SEXP hastSym);

    static unsigned getSrcPoolIndexAtOffset(SEXP hastSym, int offset);
    static rir::Code * getCodeObjectAtOffset(SEXP hastSym, int offset);
    static rir::DispatchTable * getVtableObjectAtOffset(SEXP hastSym, int offset);

    static bool isAnonEnv(SEXP env);
    static SEXP getHast(SEXP body, SEXP env);

    static void populateTypeFeedbackData(SEXP container, DispatchTable* vtab);

    static void populateOtherFeedbackData(SEXP container, DispatchTable* vtab);

    static void serializerCleanup();
};
}
