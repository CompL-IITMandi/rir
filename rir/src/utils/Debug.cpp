#include "Debug.h"
#include <iostream>

void printSpace(const unsigned int & space, std::ostream& out) {
    for (unsigned int i = 0; i < space; i++) {
        out << " ";
    }
}