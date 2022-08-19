#pragma once

#include "R/r.h"
#include <string>
#include "runtime/DispatchTable.h"

namespace rir {
    class RshSerializer {
        public:
        static void serializeFunction(SEXP hast, const unsigned & indexOffset, const std::string & name, SEXP cData, bool & serializerError);
        static void populateTypeFeedbackData(SEXP container, DispatchTable * vtab);
    };
}
