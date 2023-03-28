#pragma once

#include <iostream>
#include <string>


#include "runtime/Context.h"
#include <chrono>
#include "R/r.h"

namespace rir {


class EventLogger {
    public:
        static void logTimedEvent(
            std::chrono::_V2::system_clock::time_point& timeStamp,
            const std::string & eventType,
            size_t timeInMS,
            const std::string & eventDataInJson
        );
        static void logUntimedEvent(
            const std::string & eventType,
            const std::string & eventDataInJson
        );


        static void logStats(std::string event, std::string name, double timeInMS,
            std::chrono::_V2::system_clock::time_point& timeStamp,
            const rir::Context& context,
            SEXP closure);

        static bool enabled;

        static void logDisable(SEXP hast, const rir::Context & context);
        static void logDisableL2(SEXP hast, const rir::Context & context, const std::string & l2Info);
        static void logDispatchNormal(SEXP hast, const rir::Context & context);
        static void logDispatchL2(SEXP hast, const rir::Context & context, const std::string & l2Info);
        static void logL2Check(SEXP hast, const rir::Context & context, const std::string & l2Info, const char * l2Case);
};
}
