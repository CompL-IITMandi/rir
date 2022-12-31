#include "utils/SerializerDebug.h"
#include <iostream>
#include <cstdlib>

unsigned SerializerDebug::level = std::getenv("CC_SERIALIZER_DEBUG") ? std::stoi(std::getenv("CC_SERIALIZER_DEBUG")) : 0;

void SerializerDebug::infoMessage(const std::string & msg, const unsigned & space) {

    if (level == 0) return;

    for (unsigned i = 0; i < space; i++) {
        std::cout << " ";
    }

    std::cout << msg << std::endl;

}
