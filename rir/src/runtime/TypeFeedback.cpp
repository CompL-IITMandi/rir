#include "TypeFeedback.h"

#include "R/Symbols.h"
#include "R/r.h"
#include "runtime/Code.h"
#include "runtime/Function.h"
#include "utils/Hast.h"
#include <cassert>

namespace rir {

bool ObservedCallees::record(Code* caller, SEXP callee,
                             bool invalidateWhenFull) {
    bool changed = false;
    if (taken < CounterOverflow) {
        taken++;
    }


    if (numTargets < MaxTargets) {
        int i = 0;
        for (; i < numTargets; ++i)
            if (caller->getExtraPoolEntry(targets[i]) == callee)
                break;
        if (i == numTargets) {
            changed = true;
            auto idx = caller->addExtraPoolEntry(callee);
            targets[numTargets++] = idx;
        }
    } else {
        if (invalidateWhenFull) {
            changed = true;
            invalid = true;
        }
    }
    return changed;
}

SEXP ObservedCallees::getTarget(const Code* code, size_t pos) const {
    assert(pos < numTargets);
    return code->getExtraPoolEntry(targets[pos]);
}

FeedbackOrigin::FeedbackOrigin(rir::Code* src, Opcode* p)
    : offset_((uintptr_t)p - (uintptr_t)src), srcCode_(src) {
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

void DeoptReason::record(SEXP val) const {
    srcCode()->function()->registerDeoptReason(reason);

    switch (reason) {
    case DeoptReason::Unknown:
        break;
    case DeoptReason::DeadBranchReached: {
        assert(*pc() == Opcode::record_test_);
        ObservedTest* feedback = (ObservedTest*)(pc() + 1);
        // R_TrueValue -> true
        // R_FalseValue -> false
        // other: Both
        bool stateChange = feedback->record(R_NilValue);
        if (stateChange) {
            if (Hast::l2FastcaseInvalidationCache.count(pc() + 1) > 0) {
                for (auto & f : Hast::l2FastcaseInvalidationCache[pc() + 1]) {
                    f->valid = false;
                    f->fun = nullptr;
                }
            }
            srcCode()->function()->stateVal++;
        }
        break;
    }
    case DeoptReason::Typecheck: {
        assert(*pc() == Opcode::record_type_);
        if (val == symbol::UnknownDeoptTrigger)
            break;
        ObservedValues* feedback = (ObservedValues*)(pc() + 1);
        bool stateChange = feedback->record(val);
        if (stateChange) {
            if (Hast::l2FastcaseInvalidationCache.count(pc() + 1) > 0) {
                for (auto & f : Hast::l2FastcaseInvalidationCache[pc() + 1]) {
                    f->valid = false;
                    f->fun = nullptr;
                }
            }
            srcCode()->function()->stateVal++;
        }
        if (TYPEOF(val) == PROMSXP) {
            if (PRVALUE(val) == R_UnboundValue &&
                feedback->stateBeforeLastForce < ObservedValues::promise)
                feedback->stateBeforeLastForce = ObservedValues::promise;
            else if (feedback->stateBeforeLastForce <
                     ObservedValues::evaluatedPromise)
                feedback->stateBeforeLastForce =
                    ObservedValues::evaluatedPromise;
        }
        break;
    }
    case DeoptReason::DeadCall:
    case DeoptReason::ForceAndCall:
    case DeoptReason::CallTarget: {
        assert(*pc() == Opcode::record_call_);
        if (val == symbol::UnknownDeoptTrigger)
            break;
        ObservedCallees* feedback = (ObservedCallees*)(pc() + 1);
        bool stateChange = feedback->record(srcCode(), val, true);
        if (stateChange) {
            if (Hast::l2FastcaseInvalidationCache.count(pc() + 1) > 0) {
                for (auto & f : Hast::l2FastcaseInvalidationCache[pc() + 1]) {
                    f->valid = false;
                    f->fun = nullptr;
                }
            }
            srcCode()->function()->stateVal++;
        }
        assert(feedback->taken > 0);
        break;
    }
    case DeoptReason::EnvStubMaterialized: {
        break;
    }
    }
}

} // namespace rir
