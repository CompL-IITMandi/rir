#pragma once

#include <iostream>

//#include "Rinternals.h"
#include "runtime/Context.h"
#include <chrono>

namespace rir {


class EventLogger {
    public:

        static void logStats(std::string event, std::string name, double timeInMS,
            std::chrono::_V2::system_clock::time_point& timeStamp,
            const rir::Context& context,
            SEXP closure);

};
}
