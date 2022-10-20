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
    static BC::PoolIdx createWorklistElement(const char * pathPrefix, SEXP vtabContainer, const int & versioningInfo, const int & counter, unsigned long context);

    //
    // 0: Path prefix to load [bc|pool]
    //
    static const char * getPathPrefix(SEXP);

    //
    // 1: Vtable container
    //
    static SEXP getVtableContainer(SEXP);

    //
    // 2: Versioning info
    //
    static int getVersioningInfo(SEXP);

    //
    // 3: Counter
    //
    static int * getCounter(SEXP);

    // //
    // // 4: Nargs?
    // //
    // static void addNumArgs(SEXP, unsigned);
    // static unsigned * getNumArgs(SEXP);

    //
    // 5: Context
    //
    static unsigned long getContext(SEXP);

    //
    // 6: TFSlotInfo
    //
    static void addTFSlotInfo(SEXP, SEXP);
    static SEXP getTFSlotInfo(SEXP);

    //
    // 7: FunTFInfo
    //
    static void addFunTFInfo(SEXP, SEXP);
    static SEXP getFunTFInfo(SEXP);


    //
    // 8: GTFSlotInfo
    //
    static void addGTFSlotInfo(SEXP, SEXP);
    static SEXP getGTFSlotInfo(SEXP);

    //
    // 9: GFunTFInfo
    //
    static void addGFunTFInfo(SEXP, SEXP);
    static SEXP getGFunTFInfo(SEXP);


    static void remove(BC::PoolIdx);

    static void print(BC::PoolIdx idx, const int & space);
    static void print(SEXP, const int & space);
};

//
// OptUnlockingElement
//
class OptUnlockingElement {
  public:
    static BC::PoolIdx createOptWorklistElement(unsigned nargs, SEXP unlockingElement);

    static unsigned * getNumArgs(SEXP);

    static SEXP getUE(SEXP);

    static void print(BC::PoolIdx idx, const int & space);
    static void print(SEXP, const int & space);
};

//
// Mapping from hast -> vector of UnlockingElements
//  Only separated out for logical reasons,
//    Worklist1 is worked on when Bytecode is compiled
//    Worklist2 is worked on when elements are inserted into the dispatch table
//
class Worklist1 {
  public:
  static std::unordered_map<SEXP, std::vector<BC::PoolIdx>> worklist;

  static void remove(SEXP);
};

class Worklist2 {
  public:
  static std::unordered_map<SEXP, std::vector<BC::PoolIdx>> worklist;

  static void remove(SEXP);
};

}
