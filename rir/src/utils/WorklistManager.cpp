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
    availableMetas[hastSym] = Pool::insert(ddContainer);
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
BC::PoolIdx UnlockingElement::createWorklistElement(const char *  pathPrefix, SEXP vtabContainer, const int & versioningInfo, const int & counter) {
    SEXP store;
    Protect protecc;
    protecc(store = Rf_allocVector(VECSXP, 4));


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

    auto res = Pool::insert(store);

    return res;
}

const char * UnlockingElement::getPathPrefix(BC::PoolIdx idx) {

    SEXP pathPrefixCont = generalUtil::getSEXP(Pool::get(idx), 0);
    return CHAR(pathPrefixCont);
}

SEXP UnlockingElement::getVtableContainer(BC::PoolIdx idx) { return generalUtil::getSEXP(Pool::get(idx), 1); }

int UnlockingElement::getVersioningInfo(BC::PoolIdx idx) { return generalUtil::getInt(Pool::get(idx), 2); }

int * UnlockingElement::getCounter(BC::PoolIdx idx) {
    auto counterStore = generalUtil::getSEXP(Pool::get(idx), 3);
    return (int *) DATAPTR(counterStore);
}

void UnlockingElement::print(BC::PoolIdx idx, const int & space) {
    generalUtil::printSpace(space);
    std::cout << "Unlocking Element" << std::endl;

    generalUtil::printSpace(space + 2);
    std::cout << "PathPrefix: " << getPathPrefix(idx) << std::endl;

    generalUtil::printSpace(space + 2);
    std::cout << "getVtableContainer: " << getVtableContainer(idx) << std::endl;

    generalUtil::printSpace(space + 2);
    std::cout << "Versioning: " << getVersioningInfo(idx) << std::endl;

    generalUtil::printSpace(space + 2);
    std::cout << "Counter: " << *getCounter(idx) << std::endl;

}


}
