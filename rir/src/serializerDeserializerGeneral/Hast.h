#pragma once

#include "R/r.h"
#include "runtime/DispatchTable.h"

namespace rir {

// Instead of pair I use struct because its more readable? is it expensive?
struct hastAndIndex {
    SEXP hast;
    unsigned int index;
};

hastAndIndex getHastAndIndex(unsigned src, bool constantPool = false);
void populateHastSrcData(DispatchTable* vtable, SEXP parentHast);
SEXP getHast(SEXP body, SEXP env);

void insertVTable(DispatchTable* vtable, SEXP hastSym);
void insertClosObj(SEXP clos, SEXP hastSym);

SEXP getVtableContainer(SEXP hastSym);
SEXP getVtableContainer(SEXP hastSym, int requiredOffset);
SEXP getClosContainer(SEXP hastSym);

bool readyForSerialization(SEXP hastSym);

unsigned getSrcPoolIndexAtOffset(SEXP hastSym, int requiredOffset);

bool isHastBlacklisted(SEXP hastSym);

} // namespace rir
