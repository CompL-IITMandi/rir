#include "utils/DeserializerDebug.h"
#include <iostream>
#include <cstdlib>

unsigned DeserializerDebug::level = std::getenv("CC_DESERIALIZER_DEBUG") ? std::stoi(std::getenv("CC_DESERIALIZER_DEBUG")) : 0;

void DeserializerDebug::infoMessage(const std::string & msg, const unsigned & space) {

    if (level == 0) return;

    for (unsigned i = 0; i < space; i++) {
        std::cout << " ";
    }

    std::cout << msg << std::endl;

}
