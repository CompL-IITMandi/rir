#pragma once

#include "Rinternals.h"
#include "runtime/Context.h"
#include "R/Protect.h"
#include "utils/Debug.h"
#include "utils/deserializerData.h"
namespace rir {

struct SpeculativeContextValue{
  int tag;
  uint32_t uIntVal;
  SEXP sexpVal;
};

struct SpeculativeContextPointer{
  rir::Code *code;
  Opcode *pc;
};

class deserializedMetadata {
public:
  static unsigned int getContainerSize() { return 5; }
  //
  // 0: VTable
  //
  static void addVTab(SEXP container, SEXP data) { SET_VECTOR_ELT(container, 0, data); }
  static SEXP getVTab(SEXP container) { return VECTOR_ELT(container, 0); }
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
  // 3: Counter
  //
  static void addCounter(SEXP container, unsigned data) { addTToContainer<unsigned>(container, 3, data); }
  static unsigned * getCounter(SEXP container) { return getTFromContainerPointer<unsigned>(container, 3); }
  //
  // 4: Epoch
  //
  static void addEpoch(SEXP container, SEXP data) { SET_VECTOR_ELT(container, 4, data); }
  static SEXP getEpoch(SEXP container) { return VECTOR_ELT(container, 4); }

  static void print(SEXP container, std::ostream& out, const int & space) {
    printSpace(space, out);
    out << "█═(deserializedBinary)" << std::endl;
    printSpace(space, out);
    out << "╠═(vtab=" << getVTab(container) << ")" << std::endl;
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
    unsigned * counter = getCounter(container);
    printSpace(space, out);
    out << "╠═(counter=" << *counter << ")" << std::endl;
    printSpace(space, out);
    out << "╠═(epoch=" << CHAR(PRINTNAME(getEpoch(container))) << ")" << std::endl;
  }

};
};
