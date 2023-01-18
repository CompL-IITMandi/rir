#pragma once
#include <string>

class DeserializerDebug {
    public:

    static unsigned level;

    static void infoMessage(const std::string & msg, const unsigned & space);
};
