#include "serializerData.h"

#define DEBUG_TF_INSERTIONS 0

namespace rir {

static inline int numSlots(SEXP c) {
    return Rf_length(c) / sizeof(ObservedValues);
}

void contextData::addObservedValueToVector(SEXP container, ObservedValues * observedVal) {
    // This is inefficient but not a huge concern as we do all this in the serializer run only
    SEXP tfContainer = getTF(container);

    if (TYPEOF(tfContainer) != RAWSXP) {

        #if DEBUG_TF_INSERTIONS > 0
        { // DEBUG
            std::cout << "addObservedValueToVector [BEFORE]: <>" << std::endl;
        }
        #endif

        // Create a new vector of size = 1
        SEXP store;
        PROTECT(store = Rf_allocVector(RAWSXP, sizeof(ObservedValues)));

        // Save observed value to last slot
        ObservedValues * tmp = (ObservedValues *) DATAPTR(store);
        *tmp = *observedVal;

        // Update Container
        addTF(container, store);
        UNPROTECT(1);
    } else {

        #if DEBUG_TF_INSERTIONS > 0
        { // DEBUG
            ObservedValues * tmp = (ObservedValues *) DATAPTR(tfContainer);
            std::cout << "addObservedValueToVector [BEFORE]: <";
            for (int i = 0; i < numSlots(tfContainer); i++) {
                tmp[i].print(std::cout);
                if (i + 1 != numSlots(tfContainer)) {
                    std::cout << ", ";
                }
            }
            std::cout << ">" << std::endl;
        }
        #endif

        // Create a new vector with length = oldLength+1
        auto oldLength = numSlots(tfContainer);
        SEXP store;
        PROTECT(store = Rf_allocVector(RAWSXP, sizeof(ObservedValues) * (oldLength + 1)));

        // Copy over old data
        memcpy(DATAPTR(store), DATAPTR(tfContainer), sizeof(ObservedValues) * oldLength);

        // Save observed value to last slot
        ObservedValues * tmp = (ObservedValues *) DATAPTR(store);
        tmp += oldLength;
        *tmp = *observedVal;

        // Update Container
        addTF(container, store);
        UNPROTECT(1);
    }

    #if DEBUG_TF_INSERTIONS > 0
    // Debug
    tfContainer = getTF(container);
    ObservedValues * tmp = (ObservedValues *) DATAPTR(tfContainer);
    std::cout << "addObservedValueToVector [AFTER]: <";
    for (int i = 0; i < numSlots(tfContainer); i++) {
        tmp[i].print(std::cout);
        if (i + 1 != numSlots(tfContainer)) {
            std::cout << ", ";
        }
    }
    std::cout << ">" << std::endl;
    #endif
}

};
