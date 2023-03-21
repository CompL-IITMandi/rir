#pragma once

#include <iostream>


namespace rir {


class EventLogger {
    public:

        static void logStats(std::string event, std::string name, double timeInMS);

};
}
