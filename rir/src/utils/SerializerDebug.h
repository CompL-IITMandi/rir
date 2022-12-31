#pragma once
#include <string>

class SerializerDebug {
    public:

    static unsigned level;

    static void infoMessage(const std::string & msg, const unsigned & space);
};
