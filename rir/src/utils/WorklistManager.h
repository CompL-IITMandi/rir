#pragma once

// #include "R/RVector.h"

// #include <cassert>
// #include <cstddef>

// #include "R/r.h"

#include "ir/BC_inc.h"
#include <unordered_map>

namespace rir {

//
// Mapping from hast -> Raw Deserializer Metadata
//
class GeneralWorklist {
  static std::unordered_map<SEXP, BC::PoolIdx> availableMetas;

  public:
    static void insert(SEXP ddContainer);

    static void remove(SEXP hastSym);

    static SEXP get(SEXP hastSym);

    static void print(const unsigned int & space);
};

//
// UnlockingElement
//
class UnlockingElement {
  public:
    static BC::PoolIdx createWorklistElement(const char * pathPrefix, SEXP vtabContainer, const int & versioningInfo, const int & counter);

    //
    // 0: Path prefix to load [bc|pool]
    //
    static const char * getPathPrefix(BC::PoolIdx idx);

    //
    // 1: Vtable container
    //
    static SEXP getVtableContainer(BC::PoolIdx idx);

    //
    // 2: Versioning info
    //
    static int getVersioningInfo(BC::PoolIdx idx);

    //
    // 3: Counter
    //
    static int * getCounter(BC::PoolIdx idx);

    static void print(BC::PoolIdx idx, const int & space);
};

//
// Mapping from hast -> vector of worklist indices
//
class Worklist1 {
  static std::unordered_map<SEXP, std::vector<BC::PoolIdx>> worklist1;

  public:
    static void addToWorklist() {

    }
};

}
