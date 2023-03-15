#pragma once

#include "Rinternals.h"
#include "runtime/Context.h"
#include "R/Protect.h"
#include "utils/Debug.h"
namespace rir {
class desSElement {

public:
  static size_t getContainerSize() { return 4; }
  //
  // 0: Criteria
  //
  static void addCriteria(SEXP container, SEXP data) { SET_VECTOR_ELT(container, 0, data); }
  static SEXP getCriteria(SEXP container) { return VECTOR_ELT(container, 0); }
  //
  // 1: Offset
  //
  static void addOffset(SEXP container, size_t data) { addTToContainer<size_t>(container, 1, data); }
  static size_t getOffset(SEXP container) { return getTFromContainer<size_t>(container, 1); }
  //
  // 2: Type
  //
  static void addTag(SEXP container, int data) { addTToContainer<int>(container, 2, data); }
  static int getTag(SEXP container) { return getTFromContainer<int>(container, 2); }
  //
  // 3: Store
  //
  static void addVal(SEXP container, uint32_t data) { addTToContainer<uint32_t>(container, 3, data); }
  static uint32_t getValUint(SEXP container) { return getTFromContainer<uint32_t>(container, 3); }
  static void addVal(SEXP container, SEXP data) { SET_VECTOR_ELT(container, 3, data); }
  static SEXP getValSEXP(SEXP container) { return VECTOR_ELT(container, 3); }

  static void print(SEXP container, std::ostream& out) {
    auto tag = getTag(container);
    out << "[" << CHAR(PRINTNAME(getCriteria(container))) << "," << getOffset(container) << "," << tag << ",";
    if (tag == 0 || tag == 1) {
      out << getValUint(container) << "]";
    } else {
      SEXP val = getValSEXP(container);
      if (TYPEOF(val) == INTSXP) {
        if (INTEGER(val)[0] == -1) {
          out << "NonHast]";
        } else if (INTEGER(val)[0] == -2) {
          out << "NonRirClos]";
        } else if (INTEGER(val)[0] == -3) {
          out << "NonMono]";
        } else {
          out << "Special(" << INTEGER(val)[0] << ")]";
        }
      } else {
        assert(TYPEOF(val) == SYMSXP);
        out << CHAR(PRINTNAME(val)) << "]";
      }
    }
  }

};

class deserializerBinary {
  template <typename T>
  static void addTToContainer(SEXP container, const int & index, T val) {
      rir::Protect protecc;
      SEXP store;
      protecc(store = Rf_allocVector(RAWSXP, sizeof(T)));
      T * tmp = (T *) DATAPTR(store);
      *tmp = val;

      SET_VECTOR_ELT(container, index, store);
  }

  template <typename T>
  static T getTFromContainer(SEXP container, const int & index) {
      SEXP dataContainer = VECTOR_ELT(container, index);
      T* res = (T *) DATAPTR(dataContainer);
      return *res;
  }
public:
  static unsigned int getContainerSize() { return 5; }
  //
  // 0: Offset
  //
  static void addOffset(SEXP container, int data) { addTToContainer<int>(container, 0, data); }
  static int getOffset(SEXP container) { return getTFromContainer<int>(container, 0); }
  //
  // 1: Context
  //
  static void addContext(SEXP container, unsigned long data) { addTToContainer<unsigned long>(container, 1, data); }
  static unsigned long getContext(SEXP container) { return getTFromContainer<unsigned long>(container, 1); }
  //
  // 2: SpeculativeContext
  //
  static void addSpeculativeContext(SEXP container, SEXP data) { SET_VECTOR_ELT(container, 2, data); }
  static SEXP getSpeculativeContext(SEXP container) { return VECTOR_ELT(container, 2); }
  //
  // 3: Dependencies
  //
  static void addDependencies(SEXP container, std::vector<SEXP> deps) {
    SEXP store;
    rir::Protect protecc;
    protecc(store = Rf_allocVector(VECSXP,deps.size()));
    for (size_t i = 0; i < deps.size(); i++) {
      SET_VECTOR_ELT(store, i, deps[i]);
    }

    SET_VECTOR_ELT(container, 3, store);
  }
  static SEXP getDependencies(SEXP container) { return VECTOR_ELT(container, 3); }
  //
  // 4: Epoch
  //
  static void addEpoch(SEXP container, SEXP data) { SET_VECTOR_ELT(container, 4, data); }
  static SEXP getEpoch(SEXP container) { return VECTOR_ELT(container, 4); }

  static void print(SEXP container, std::ostream& out, const int & space) {
    printSpace(space, out);
    out << "█═(deserializerBinary)" << std::endl;
    printSpace(space, out);
    out << "╠═(offset=" << getOffset(container) << ")" << std::endl;
    printSpace(space, out);
    out << "╠═(context=" << getContext(container) << "): " << rir::Context(getContext(container)) << std::endl;

    SEXP speculativeContext = getSpeculativeContext(container);
    if (speculativeContext == R_NilValue) {
      printSpace(space, out);
      out << "╠═(speculativeContext=NIL)" << std::endl;
    } else {
      printSpace(space, out);
      out << "╠═(speculativeContext=" << Rf_length(speculativeContext) << "): [ ";

      for (int i = 0; i < Rf_length(speculativeContext); i++) {
        SEXP curr = VECTOR_ELT(speculativeContext, i);
        desSElement::print(curr, out);
        out << " ";
      }
      out << "]" << std::endl;

    }

    SEXP dependencies = getDependencies(container);
    printSpace(space, out);
    out << "╠═(dependencies=" << Rf_length(dependencies) << "): [ ";

    for (int i = 0; i < Rf_length(dependencies); i++) {
      SEXP curr = VECTOR_ELT(dependencies, i);
      out << CHAR(PRINTNAME(curr)) << " ";
    }
    out << "]" << std::endl;

    printSpace(space, out);
    out << "╠═(epoch=" << CHAR(PRINTNAME(getEpoch(container))) << ")" << std::endl;


  }

};

class deserializerData {
public:
  static unsigned int getContainerSize() { return 2; }
  //
  // 0: Hast
  //
  static void addHast(SEXP container, SEXP data) { SET_VECTOR_ELT(container, 0, data); }
  static SEXP getHast(SEXP container) { return VECTOR_ELT(container, 0); }
  //
  // 1: Binaries
  //
  static void addBinaries(SEXP container, std::vector<SEXP> binaries) {
    SEXP store;
    rir::Protect protecc;
    protecc(store = Rf_allocVector(VECSXP,binaries.size()));

    for (size_t i = 0; i < binaries.size(); i++) {
      SET_VECTOR_ELT(store, i, binaries[i]);
    }

    SET_VECTOR_ELT(container, 1, store);
  }
  static SEXP getBinaries(SEXP container) { return VECTOR_ELT(container, 1); }

  static void print(SEXP container, std::ostream& out, const int & space) {
    printSpace(space, out);
    out << "█═════════(DESERIALIZER_DATA)═════════█" << std::endl;
    printSpace(space, out);
    out << "╠═(hast=" << CHAR(PRINTNAME(getHast(container))) << ")" << std::endl;
    SEXP binaries = getBinaries(container);
    for (int i = 0; i < Rf_length(binaries); i++) {
      SEXP ele = VECTOR_ELT(binaries, i);
      deserializerBinary::print(ele, std::cout, space + 2);
    }

  }
};
};
