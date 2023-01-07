#include "serializerData.h"
#include "runtime/DispatchTable.h"
#include "api.h"

#include "utils/Hast.h"

#define DEBUG_TF_INSERTIONS 0
#define DEBUG_OTHER_TF_INSERTIONS 0

namespace rir {

static inline int numSlots(SEXP c) {
    return Rf_length(c) / sizeof(ObservedValues);
}

void contextData::addObservedTestToVector(SEXP container, ObservedTest * observedVal) {
    // This is inefficient but not a huge concern as we do all this in the serializer run only
    SEXP tfContainer = getFBD(container);

    if (TYPEOF(tfContainer) != VECSXP) {

        #if DEBUG_OTHER_TF_INSERTIONS > 0
        { // DEBUG
            std::cout << "addObservedTestToVector [BEFORE]: <>" << std::endl;
        }
        #endif

        // Create a new vector of size = 1
        SEXP store;
        PROTECT(store = Rf_allocVector(VECSXP, 1));

        int sIndex = 0;

        // We only optimize if one of these is true, otherwise we dont really do much, so leaving it null
        switch (observedVal->seen) {
            case ObservedTest::OnlyTrue:
                SET_VECTOR_ELT(store, sIndex, R_dot_defined);
                break;
            case ObservedTest::OnlyFalse:
                SET_VECTOR_ELT(store, sIndex, R_dot_Method);
                break;
            default:
                SET_VECTOR_ELT(store, sIndex, R_NilValue);
                break;
        }


        // Update Container
        addFBD(container, store);
        UNPROTECT(1);
    } else {

        #if DEBUG_OTHER_TF_INSERTIONS > 0
        { // DEBUG

            std::cout << "addObservedTestToVector [BEFORE]: <";
            for (int i = 0; i < Rf_length(tfContainer); i++) {
                SEXP ele = VECTOR_ELT(tfContainer, i);
                if (ele == R_NilValue) {
                    std::cout << "NIL, ";
                } else if (ele == R_dot_defined) {
                    std::cout << "T, ";
                } else if (ele == R_dot_Method) {
                    std::cout << "F, ";
                } else if (TYPEOF(ele) == VECSXP){
                    auto hast = VECTOR_ELT(ele, 0);
                    auto index = Rf_asInteger(VECTOR_ELT(ele, 1));
                    std::cout << "(" << CHAR(PRINTNAME(hast)) << "," << index << "), ";
                } else {
                    std::cout << "UN, ";
                }
            }
            std::cout << ">" << std::endl;
        }
        #endif

        // Create a new vector with length = oldLength+1
        auto oldLength = Rf_length(tfContainer);
        SEXP store;
        PROTECT(store = Rf_allocVector(VECSXP, oldLength + 1));

        // Copy over old data
        memcpy(DATAPTR(store), DATAPTR(tfContainer), sizeof(SEXP) * oldLength);

        int sIndex = oldLength;

        // We only optimize if one of these is true, otherwise we dont really do much, so leaving it null
        switch (observedVal->seen) {
            case ObservedTest::OnlyTrue:
                SET_VECTOR_ELT(store, sIndex, R_dot_defined);
                break;
            case ObservedTest::OnlyFalse:
                SET_VECTOR_ELT(store, sIndex, R_dot_Method);
                break;
            default:
                SET_VECTOR_ELT(store, sIndex, R_NilValue);
                break;
        }

        // Update Container
        addFBD(container, store);
        UNPROTECT(1);
    }

    #if DEBUG_OTHER_TF_INSERTIONS > 0
    // Debug
    tfContainer = getFBD(container);
    std::cout << "  TYPEOF(FBD): " << TYPEOF(tfContainer) << std::endl;
    if (TYPEOF(tfContainer) == VECSXP) {
        std::cout << "  VECSXP(len): " << Rf_length(tfContainer) << std::endl;
    }
    std::cout << "addObservedTestToVector [AFTER]: <";
    for (int i = 0; i < Rf_length(tfContainer); i++) {
        SEXP ele = VECTOR_ELT(tfContainer, i);
        if (ele == R_NilValue) {
            std::cout << "NIL, ";
        } else if (ele == R_TrueValue) {
            std::cout << "T, ";
        } else if (ele == R_FalseValue) {
            std::cout << "F, ";
        } else if (TYPEOF(ele) == VECSXP){
            auto hast = VECTOR_ELT(ele, 0);
            auto index = Rf_asInteger(VECTOR_ELT(ele, 1));
            std::cout << "(" << CHAR(PRINTNAME(hast)) << "," << index << "), ";
        } else {
            std::cout << "UN, ";
        }
    }
    std::cout << ">" << std::endl;
    #endif
}


void contextData::addObservedCallSiteInfo(SEXP container, ObservedCallees * feedback, rir::Code * srcCode) {
    // This is inefficient but not a huge concern as we do all this in the serializer run only
    SEXP tfContainer = getFBD(container);

    if (TYPEOF(tfContainer) != VECSXP) {

        #if DEBUG_OTHER_TF_INSERTIONS > 0
        { // DEBUG
            std::cout << "addObservedCallSiteInfo [BEFORE]: <>" << std::endl;
        }
        #endif

        // Create a new vector of size = 1
        SEXP store;
        PROTECT(store = Rf_allocVector(VECSXP, 1));

        int sIndex = 0;

        // If the feedback was monomorphic, then we store the required hast
        if (feedback->numTargets == 1) {
            auto target = feedback->getTarget(srcCode, 0);
            if (TYPEOF(target) == CLOSXP && TYPEOF(BODY(target)) == EXTERNALSXP && DispatchTable::check(BODY(target))) {
                auto targetVtab = DispatchTable::unpack(BODY(target));
                auto targetCode = targetVtab->baseline()->body();

                auto hastInfo = Hast::getHastInfo(targetCode->src, true);
                if (hastInfo.isValid()) {
                    SEXP ss;
                    PROTECT(ss = Rf_allocVector(VECSXP, 2));
                    SET_VECTOR_ELT(ss, 0, hastInfo.hast);
                    SET_VECTOR_ELT(ss, 1, Rf_ScalarInteger(hastInfo.offsetIndex));

                    SET_VECTOR_ELT(store, sIndex, ss);

                    UNPROTECT(1);
                } else {
                    SET_VECTOR_ELT(store, sIndex, R_NilValue);
                }
            } else {
                // std::cout << "Serializer Feedback, unsupported call site is type: " << TYPEOF(target) << std::endl;
                // if (TYPEOF(target) == CLOSXP) {
                //     std::cout << "  TYPEOF(BODY(target))" << TYPEOF(BODY(target)) << std::endl;
                //     std::cout << "  Code::check(BODY(target))" << Code::check(BODY(target)) << std::endl;
                //     std::cout << "  DispatchTable::check(BODY(target))" << DispatchTable::check(BODY(target)) << std::endl;
                // }
                SET_VECTOR_ELT(store, sIndex, R_NilValue);
            }
        } else {
            SET_VECTOR_ELT(store, sIndex, R_NilValue);
        }


        // Update Container
        addFBD(container, store);
        UNPROTECT(1);
    } else {

        #if DEBUG_OTHER_TF_INSERTIONS > 0
        { // DEBUG

            std::cout << "addObservedCallSiteInfo [BEFORE]: <";
            for (int i = 0; i < Rf_length(tfContainer); i++) {
                SEXP ele = VECTOR_ELT(tfContainer, i);
                if (ele == R_NilValue) {
                    std::cout << "NIL, ";
                } else if (ele == R_dot_defined) {
                    std::cout << "T, ";
                } else if (ele == R_dot_Method) {
                    std::cout << "F, ";
                } else if (TYPEOF(ele) == VECSXP){
                    auto hast = VECTOR_ELT(ele, 0);
                    auto index = Rf_asInteger(VECTOR_ELT(ele, 1));
                    std::cout << "(" << CHAR(PRINTNAME(hast)) << "," << index << "), ";
                } else {
                    std::cout << "UN, ";
                }
            }
            std::cout << ">" << std::endl;
        }
        #endif

        // Create a new vector with length = oldLength+1
        auto oldLength = Rf_length(tfContainer);
        SEXP store;
        PROTECT(store = Rf_allocVector(VECSXP, oldLength + 1));

        // Copy over old data
        memcpy(DATAPTR(store), DATAPTR(tfContainer), sizeof(SEXP) * oldLength);

        int sIndex = oldLength;

        // If the feedback was monomorphic, then we store the required hast
        if (feedback->numTargets == 1) {
            auto target = feedback->getTarget(srcCode, 0);
            if (TYPEOF(target) == CLOSXP && TYPEOF(BODY(target)) == EXTERNALSXP && DispatchTable::check(BODY(target))) {
                auto targetVtab = DispatchTable::unpack(BODY(target));
                auto targetCode = targetVtab->baseline()->body();

                auto hastInfo = Hast::getHastInfo(targetCode->src, true);
                if (hastInfo.isValid()) {
                    SEXP ss;
                    PROTECT(ss = Rf_allocVector(VECSXP, 2));
                    SET_VECTOR_ELT(ss, 0, hastInfo.hast);
                    SET_VECTOR_ELT(ss, 1, Rf_ScalarInteger(hastInfo.offsetIndex));

                    SET_VECTOR_ELT(store, sIndex, ss);

                    UNPROTECT(1);
                } else {
                    SET_VECTOR_ELT(store, sIndex, R_NilValue);
                }

            } else {
                // std::cout << "Serializer Feedback, unsupported call site is type: " << TYPEOF(target) << std::endl;
                // if (TYPEOF(target) == CLOSXP) {
                //     std::cout << "  TYPEOF(BODY(target))" << TYPEOF(BODY(target)) << std::endl;
                //     std::cout << "  Code::check(BODY(target))" << Code::check(BODY(target)) << std::endl;
                //     std::cout << "  DispatchTable::check(BODY(target))" << DispatchTable::check(BODY(target)) << std::endl;
                // }
                SET_VECTOR_ELT(store, sIndex, R_NilValue);
            }
        } else {
            SET_VECTOR_ELT(store, sIndex, R_NilValue);
        }

        // // Save observed value to last slot
        // ObservedValues * tmp = (ObservedValues *) DATAPTR(store);
        // tmp += oldLength;
        // *tmp = *observedVal;

        // Update Container
        addFBD(container, store);
        UNPROTECT(1);
    }

    #if DEBUG_OTHER_TF_INSERTIONS > 0
    // Debug
    tfContainer = getFBD(container);
    std::cout << "  TYPEOF(FBD): " << TYPEOF(tfContainer) << std::endl;
    if (TYPEOF(tfContainer) == VECSXP) {
        std::cout << "  VECSXP(len): " << Rf_length(tfContainer) << std::endl;
    }
    std::cout << "addObservedCallSiteInfo [AFTER]: <";
    for (int i = 0; i < Rf_length(tfContainer); i++) {
        SEXP ele = VECTOR_ELT(tfContainer, i);
        if (ele == R_NilValue) {
            std::cout << "NIL, ";
        } else if (ele == R_TrueValue) {
            std::cout << "T, ";
        } else if (ele == R_FalseValue) {
            std::cout << "F, ";
        } else if (TYPEOF(ele) == VECSXP){
            auto hast = VECTOR_ELT(ele, 0);
            auto index = Rf_asInteger(VECTOR_ELT(ele, 1));
            std::cout << "(" << CHAR(PRINTNAME(hast)) << "," << index << "), ";
        } else {
            std::cout << "UN, ";
        }
    }
    std::cout << ">" << std::endl;
    #endif
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
