#include "utils/WorklistManager.h"
#include "utils/Pool.h"
#include "utils/deserializerData.h"

#include "R/Protect.h"
namespace rir {

//
// GeneralWorklist
//
std::unordered_map<SEXP, BC::PoolIdx> GeneralWorklist::availableMetas;

void GeneralWorklist::insert(SEXP ddContainer) {
    SEXP hastSym = deserializerData::getHast(ddContainer);
    auto idx = Pool::makeSpace();
    availableMetas[hastSym] = idx;
    Pool::patch(idx, ddContainer);
}

void GeneralWorklist::remove(SEXP hastSym) {
    availableMetas.erase(hastSym);
}

SEXP GeneralWorklist::get(SEXP hastSym) {
    if (availableMetas.count(hastSym)) {
        return Pool::get(availableMetas[hastSym]);
    } else {
        return nullptr;
    }
}

void GeneralWorklist::print(const unsigned int & space) {
    generalUtil::printSpace(space);
    std::cout << "Deserializer Map" << std::endl;

    for (auto & ele : availableMetas) {
        generalUtil::printSpace(space + 2);
        std::cout << "HAST: " << CHAR(PRINTNAME(ele.first)) << std::endl;

        deserializerData::print(Pool::get(ele.second), space + 4);
    }
}

//
// UnlockingElement
//
BC::PoolIdx UnlockingElement::createWorklistElement(const char *  pathPrefix, SEXP vtabContainer, const int & versioningInfo, const int & counter, unsigned long context) {
    SEXP store;
    Protect protecc;
    protecc(store = Rf_allocVector(VECSXP, 7));


    SEXP pathPrefixCont;
    protecc(pathPrefixCont = Rf_mkChar(pathPrefix));
    generalUtil::addSEXP(store, pathPrefixCont, 0);

    generalUtil::addSEXP(store, vtabContainer, 1);

    generalUtil::addInt(store, versioningInfo, 2);


    SEXP counterStore;
    protecc(counterStore = Rf_allocVector(RAWSXP, sizeof(int)));
    int * tmp = (int *) DATAPTR(counterStore);
    *tmp = counter;
    generalUtil::addSEXP(store, counterStore, 3);

    // generalUtil::addSEXP(store, R_NilValue, 4);

    generalUtil::addUnsignedLong(store, context, 4);

    generalUtil::addSEXP(store, R_NilValue, 5);
    generalUtil::addSEXP(store, R_NilValue, 6);

    BC::PoolIdx ueIdx = Pool::makeSpace();
    Pool::patch(ueIdx, store);

    return ueIdx;
}

const char * UnlockingElement::getPathPrefix(SEXP container) {
    SEXP pathPrefixCont = generalUtil::getSEXP(container, 0);
    return CHAR(pathPrefixCont);
}

SEXP UnlockingElement::getVtableContainer(SEXP container) { return generalUtil::getSEXP(container, 1); }

int UnlockingElement::getVersioningInfo(SEXP container) { return generalUtil::getInt(container, 2); }

int * UnlockingElement::getCounter(SEXP container) {
    auto counterStore = generalUtil::getSEXP(container, 3);
    return (int *) DATAPTR(counterStore);
}

// void UnlockingElement::addNumArgs(SEXP container, unsigned numArgs) {
//     Protect protecc;

//     SEXP store;
//     protecc(store = Rf_allocVector(RAWSXP, sizeof(unsigned)));
//     unsigned * tmp = (unsigned *) DATAPTR(store);
//     *tmp = numArgs;

//     generalUtil::addSEXP(container, store, 4);
// }

// unsigned * UnlockingElement::getNumArgs(SEXP container) {
//     auto store = generalUtil::getSEXP(container, 4);
//     return (store == R_NilValue) ? nullptr : (unsigned *) DATAPTR(store);
// }

// static bool containsNArgs(SEXP container) {
//     return generalUtil::getSEXP(container, 4) == R_NilValue ? false : true;
// }

unsigned long UnlockingElement::getContext(SEXP container) {
    return generalUtil::getUnsignedLong(container, 4);
}

void UnlockingElement::addTFSlotInfo(SEXP container, SEXP TFSlot) {
    generalUtil::addSEXP(container, TFSlot, 5);
}

SEXP UnlockingElement::getTFSlotInfo(SEXP container) {
    return generalUtil::getSEXP(container, 5);
}


void UnlockingElement::addFunTFInfo(SEXP container, SEXP FunTF) {
    generalUtil::addSEXP(container, FunTF, 6);
}

SEXP UnlockingElement::getFunTFInfo(SEXP container) {
    return generalUtil::getSEXP(container, 6);
}

void UnlockingElement::remove(BC::PoolIdx ueIdx) {
    Pool::patch(ueIdx, R_NilValue);
}

void UnlockingElement::print(BC::PoolIdx idx, const int & space) {
    print(Pool::get(idx), space);
}

void UnlockingElement::print(SEXP container, const int & space) {
    generalUtil::printSpace(space);
    std::cout << "[Unlocking Element]" << std::endl;

    generalUtil::printSpace(space + 2);
    std::cout << "├─(ENTRY 0, PathPrefix    ): " << getPathPrefix(container) << std::endl;

    generalUtil::printSpace(space + 2);
    std::cout << "├─(ENTRY 1, VtabContainer ): " << getVtableContainer(container) << std::endl;

    generalUtil::printSpace(space + 2);
    std::cout << "├─(ENTRY 2, Versioning    ): " << getVersioningInfo(container) << std::endl;

    generalUtil::printSpace(space + 2);
    std::cout << "├─(ENTRY 3, Counter       ): " << *getCounter(container) << std::endl;

    // generalUtil::printSpace(space + 2);
    // if (containsNArgs(container)) {
    //     std::cout << "├─(ENTRY 4, numArgs       ): " << *getNumArgs(container) << std::endl;
    // } else {
    //     std::cout << "├─(ENTRY 4, numArgs       ): NA" << std::endl;
    // }

    generalUtil::printSpace(space + 2);
    std::cout << "├─(ENTRY 4, context       ): (" << getContext(container) << ") " << Context(getContext(container)) << std::endl;

    if (getTFSlotInfo(container) == R_NilValue) {
        generalUtil::printSpace(space + 2);
        std::cout << "└─(No Type Versioning Info)" << std::endl;
    } else {
        SEXP slotsInfo = getTFSlotInfo(container);

        generalUtil::printSpace(space + 2);
        std::cout << "├─(ENTRY 5, TF Slot Idx  ): [ ";
        for (int i = 0; i < Rf_length(slotsInfo); i++) {
            std::cout << Rf_asInteger(VECTOR_ELT(slotsInfo, i)) << " ";
        }
        std::cout << "]" << std::endl;

        SEXP funTF = getFunTFInfo(container);
        generalUtil::printSpace(space + 2);
        std::cout << "└─(ENTRY 6, Fun TF Data  ): [ ";
        for (int i = 0; i < Rf_length(funTF); i++) {
            auto ele = generalUtil::getUint32t(funTF, i);
            std::cout << ele << " ";
        }
        std::cout << "]" << std::endl;

    }
}

//
// OptUnlockingElement
//
BC::PoolIdx OptUnlockingElement::createOptWorklistElement(unsigned numArgs, SEXP unlockingElement) {
    SEXP store;
    Protect protecc;
    protecc(store = Rf_allocVector(VECSXP, 2));


    SEXP numArgsStore;
    protecc(numArgsStore = Rf_allocVector(RAWSXP, sizeof(unsigned)));
    unsigned * tmp = (unsigned *) DATAPTR(numArgsStore);
    *tmp = numArgs;

    generalUtil::addSEXP(store, numArgsStore, 0);

    generalUtil::addSEXP(store, unlockingElement, 1);

    BC::PoolIdx ueIdx = Pool::makeSpace();
    Pool::patch(ueIdx, store);

    return ueIdx;


}

unsigned * OptUnlockingElement::getNumArgs(SEXP container) {
    auto store = generalUtil::getSEXP(container, 0);
    return (store == R_NilValue) ? nullptr : (unsigned *) DATAPTR(store);
}

SEXP OptUnlockingElement::getUE(SEXP container) {
    return generalUtil::getSEXP(container, 1);
}

void OptUnlockingElement::print(BC::PoolIdx idx, const int & space) {
    print(Pool::get(idx), space);
}

void OptUnlockingElement::print(SEXP container, const int & space) {
    generalUtil::printSpace(space);
    std::cout << "[Unlocking Element]" << std::endl;

    generalUtil::printSpace(space + 2);
    std::cout << "├─(ENTRY 0, numArgs       ): " << *getNumArgs(container) << std::endl;

    SEXP uEleContainer = getUE(container);

    UnlockingElement::print(uEleContainer, space + 4);
}

//
// Worklists
//

std::unordered_map<SEXP, std::vector<BC::PoolIdx>> Worklist1::worklist;
std::unordered_map<SEXP, std::vector<BC::PoolIdx>> Worklist2::worklist;

void Worklist1::remove(SEXP key) {
    worklist.erase(key);
}

void Worklist2::remove(SEXP key) {
    worklist.erase(key);
}

}
