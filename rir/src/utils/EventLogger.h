#pragma once

#include <iostream>
#include "R/r.h"
#include <string>
#include "runtime/Context.h"

namespace rir {


class EventLogger {
    public:

        static void logStats(std::string event, std::string name, double timeInMS);
        static void logDisable(SEXP hast, const rir::Context & context);
        static void logDisableL2(SEXP hast, const rir::Context & context, const std::string & l2Info);
        static void logDispatchNormal(SEXP hast, const rir::Context & context);
        static void logDispatchL2(SEXP hast, const rir::Context & context, const std::string & l2Info);
        static void logL2Check(SEXP hast, const rir::Context & context, const std::string & l2Info, const char * l2Case);
};
}
