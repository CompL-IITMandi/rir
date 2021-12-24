#include "TypeFeedback.h"
#include "R/r.h"
#include "runtime/Code.h"

#include "patches.h"

#include <cassert>

namespace rir {

void ObservedCallees::record(Code* caller, SEXP callee) {
    if (taken < CounterOverflow)
        taken++;
    if (numTargets < MaxTargets) {
        int i = 0;
        for (; i < numTargets; ++i)
            if (caller->getExtraPoolEntry(targets[i]) == callee)
                break;
        if (i == numTargets) {
            auto idx = caller->addExtraPoolEntry(callee);
            targets[numTargets++] = idx;
        }
    }
}

SEXP ObservedCallees::getTarget(const Code* code, size_t pos) const {
    assert(pos < numTargets);
    return code->getExtraPoolEntry(targets[pos]);
}

FeedbackOrigin::FeedbackOrigin(rir::Code* src, Opcode* p)
    : offset_(
        #if TRY_PATCH_DEOPTREASON_PC == 1
        (uintptr_t)p - (uintptr_t)src->code()
        #else
        (uintptr_t)p - (uintptr_t)src
        #endif
        ), srcCode_(src) {
    if (p) {
        assert(p >= src->code());
        assert(p < src->endCode());
        assert(pc() == p);
    }
}

DeoptReason::DeoptReason(const FeedbackOrigin& origin,
                         DeoptReason::Reason reason)
    : reason(reason), origin(origin) {
    switch (reason) {
    case DeoptReason::Typecheck:
    case DeoptReason::DeadCall:
    case DeoptReason::CallTarget:
    case DeoptReason::ForceAndCall:
    case DeoptReason::DeadBranchReached: {
        assert(pc());
        auto o = *pc();
        assert(o == Opcode::record_call_ || o == Opcode::record_type_ ||
               o == Opcode::record_test_);
        break;
    }
    case DeoptReason::Unknown:
    case DeoptReason::EnvStubMaterialized:
        break;
    }
}

Opcode* FeedbackOrigin::pc() const {
    if (offset_ == 0)
        return nullptr;
    #if TRY_PATCH_DEOPTREASON_PC == 1
    return (Opcode*)((uintptr_t)srcCode()->code() + offset_);
    #else
    return (Opcode*)((uintptr_t)srcCode() + offset_);
    #endif
}

} // namespace rir
