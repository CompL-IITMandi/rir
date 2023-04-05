#ifndef RIR_RUNTIME_FEEDBACK
#define RIR_RUNTIME_FEEDBACK

#include "R/r.h"
#include "common.h"
#include <array>
#include <cstdint>
#include <iostream>

namespace rir {

struct Code;

#pragma pack(push)
#pragma pack(1)

struct ObservedCallees {
    static constexpr unsigned CounterBits = 29;
    static constexpr unsigned CounterOverflow = (1 << CounterBits) - 1;
    static constexpr unsigned TargetBits = 2;
    static constexpr unsigned MaxTargets = (1 << TargetBits) - 1;

    // numTargets is sized such that the largest number it can hold is
    // MaxTargets. If it is set to MaxTargets then the targets array is full. We
    // do not distinguish between the case where we have seen MaxTarget
    // different targets and the case where we have seen more than that.
    // Effectively this means we have seen MaxTargets or more.
    uint32_t numTargets : TargetBits;
    uint32_t taken : CounterBits;
    uint32_t invalid : 1;

    bool record(Code* caller, SEXP callee, bool invalidateWhenFull = false);
    SEXP getTarget(const Code* code, size_t pos) const;

    std::array<unsigned, MaxTargets> targets;
};
static_assert(sizeof(ObservedCallees) == 4 * sizeof(uint32_t),
              "Size needs to fit inside a record_ bc immediate args");

inline bool fastVeceltOk(SEXP vec) {
    return !Rf_isObject(vec) &&
           (ATTRIB(vec) == R_NilValue || (TAG(ATTRIB(vec)) == R_DimSymbol &&
                                          CDR(ATTRIB(vec)) == R_NilValue));
}

struct ObservedTest {
    enum { None, OnlyTrue, OnlyFalse, Both };
    uint32_t seen : 2;
    uint32_t unused : 30;

    ObservedTest() : seen(0), unused(0) {}

    inline bool record(SEXP e) {
        bool changed = false;
        if (e == R_TrueValue) {
            if (seen == None) {
                if (seen != OnlyTrue) {
                    seen = OnlyTrue;
                    changed = true;
                }
            }
            else if (seen != OnlyTrue) {
                if (seen != Both) {
                    seen = Both;
                    changed = true;
                }
            }
            return changed;
        }
        if (e == R_FalseValue) {
            if (seen == None) {
                if (seen != OnlyFalse) {
                    seen = OnlyFalse;
                    changed = true;
                }
            }
            else if (seen != OnlyFalse) {
                if (seen != Both) {
                    seen = Both;
                    changed = true;
                }
            }
            return changed;
        }
        if (seen == Both) {
            return false;
        }
        seen = Both;
        return true;
    }
};
static_assert(sizeof(ObservedTest) == sizeof(uint32_t),
              "Size needs to fit inside a record_ bc immediate args");

struct ObservedValues {

    enum StateBeforeLastForce {
        unknown,
        value,
        evaluatedPromise,
        promise,
    };

    static constexpr unsigned MaxTypes = 3;
    uint8_t numTypes : 2;
    uint8_t stateBeforeLastForce : 2;
    uint8_t notScalar : 1;
    uint8_t attribs : 1;
    uint8_t object : 1;
    uint8_t notFastVecelt : 1;

    std::array<uint8_t, MaxTypes> seen;

    ObservedValues() {
        // implicitly happens when writing bytecode stream...
        memset(this, 0, sizeof(ObservedValues));
    }

    void reset() { *this = ObservedValues(); }

    static bool arePropertiesCompatible(ObservedValues & expected, ObservedValues & observed) {
        // This is a breaking property as code may be optimized assuming a scalar
        if (expected.notScalar == 1 && observed.notScalar != 1) return false;
        if (expected.attribs == 1 && observed.attribs != 1) return false;
        if (expected.object == 1 && observed.object != 1) return false;
        if (expected.notFastVecelt == 1 && observed.notFastVecelt != 1) return false;
        return true;
    }

    static bool isCompatible(ObservedValues & expected, ObservedValues & observed) {
        if (expected.numTypes == 0) {
            return true;
        }
        // 1. Case when only one type is expected
        if (expected.numTypes == 1) {
            if (observed.numTypes == 0) {
                // Runtime has not yet seen this slot, be conservative and let this slot get updated before dispatch
                return false;
            }
            if (observed.numTypes == 1) {
                bool isObservedTypeSame = expected.seen[0] == observed.seen[0];
                if (!isObservedTypeSame) return false;
                return arePropertiesCompatible(expected, observed);
            }
            // If the expectation is a single type but runtime has seen more, its a good idea to dispatch to more generic versions
            return false;
        }
        // If more than one type are seen, we probably don't optimize on type but on the basis of their properties
        return arePropertiesCompatible(expected, observed);
    }

    void print(std::ostream& out) const {
        if (numTypes) {
            for (size_t i = 0; i < numTypes; ++i) {
                out << Rf_type2char(seen[i]);
                if (i != (unsigned)numTypes - 1)
                    out << ", ";
            }
            out << " (" << (object ? "o" : "") << (attribs ? "a" : "")
                << (notFastVecelt ? "v" : "") << (!notScalar ? "s" : "") << ")";
            if (stateBeforeLastForce !=
                ObservedValues::StateBeforeLastForce::unknown) {
                out << " | "
                    << ((stateBeforeLastForce ==
                         ObservedValues::StateBeforeLastForce::value)
                            ? "value"
                            : (stateBeforeLastForce ==
                               ObservedValues::StateBeforeLastForce::
                                   evaluatedPromise)
                                  ? "evaluatedPromise"
                                  : "promise");
            }
        } else {
            out << "<?>";
        }
    }

        inline bool record(SEXP e) {

        // Set attribs flag for every object even if the SEXP does  not
        // have attributes. The assumption used to be that e having no
        // attributes implies that it is not an object, but this is not
        // the case in some very specific cases:
        //     > df <- data.frame(x=ts(c(41,42,43)), y=c(61,62,63))
        //     > mf <- model.frame(df)
        //     > .Internal(inspect(mf[["x"]]))
        //     @56546cb06390 14 REALSXP g0c3 [OBJ,NAM(2)] (len=3, tl=0) 41,42,43

        bool changed = false;

        auto oldNotScalar = notScalar;
        auto oldObject = object;
        auto oldAttribs = attribs;
        auto oldNotFastVecelt = notFastVecelt;

        notScalar = notScalar || XLENGTH(e) != 1;
        object = object || Rf_isObject(e);
        attribs = attribs || object || ATTRIB(e) != R_NilValue;
        notFastVecelt = notFastVecelt || !fastVeceltOk(e);

        changed = (oldNotScalar != notScalar) ||
                  (oldObject != object) ||
                  (oldAttribs != attribs) ||
                  (oldNotFastVecelt != notFastVecelt);

        uint8_t type = TYPEOF(e);
        if (numTypes < MaxTypes) {
            int i = 0;
            for (; i < numTypes; ++i) {
                if (seen[i] == type)
                    break;
            }
            if (i == numTypes) {
                changed = true;
                seen[numTypes++] = type;
            }
        }

        return changed;
    }
};
static_assert(sizeof(ObservedValues) == sizeof(uint32_t),
              "Size needs to fit inside a record_ bc immediate args");

enum class Opcode : uint8_t;

struct FeedbackOrigin {
  private:

  public:
    uint32_t offset_ = 0;
    Code* srcCode_ = nullptr;
    FeedbackOrigin() {}
    FeedbackOrigin(rir::Code* src, Opcode* pc);

    Opcode* pc() const {
        if (offset_ == 0)
            return nullptr;
        return (Opcode*)((uintptr_t)srcCode() + offset_);
    }
    uint32_t offset() const { return offset_; }
    Code* srcCode() const { return srcCode_; }
    void srcCode(Code* src) { srcCode_ = src; }

    bool operator==(const FeedbackOrigin& other) const {
        return offset_ == other.offset_ && srcCode_ == other.srcCode_;
    }
};

struct DeoptReason {
  public:
    enum Reason : uint32_t {
        Unknown,
        Typecheck,
        DeadCall,
        CallTarget,
        ForceAndCall,
        EnvStubMaterialized,
        DeadBranchReached,
    };

    DeoptReason::Reason reason;
    FeedbackOrigin origin;

    DeoptReason(const FeedbackOrigin& origin, DeoptReason::Reason reason);

    Code* srcCode() const { return origin.srcCode(); }
    Opcode* pc() const { return origin.pc(); }

    bool operator==(const DeoptReason& other) const {
        return reason == other.reason && origin == other.origin;
    }

    friend std::ostream& operator<<(std::ostream& out,
                                    const DeoptReason& reason) {
        switch (reason.reason) {
        case Typecheck:
            out << "Typecheck";
            break;
        case DeadCall:
            out << "DeadCall";
            break;
        case CallTarget:
            out << "CallTarget";
            break;
        case ForceAndCall:
            out << "ForceAndCall";
            break;
        case EnvStubMaterialized:
            out << "EnvStubMaterialized";
            break;
        case DeadBranchReached:
            out << "DeadBranchReached";
            break;
        case Unknown:
            out << "Unknown";
            break;
        }
        out << "@" << (void*)reason.pc();
        return out;
    }

    static DeoptReason unknown() {
        return DeoptReason(FeedbackOrigin(0, 0), Unknown);
    }

    void record(SEXP val) const;

    DeoptReason() = delete;

  private:
    friend struct std::hash<rir::DeoptReason>;
};
static_assert(sizeof(DeoptReason) == 4 * sizeof(uint32_t),
              "Size needs to fit inside a record_deopt_ bc immediate args");

#pragma pack(pop)

} // namespace rir

namespace std {
template <>
struct hash<rir::DeoptReason> {
    std::size_t operator()(const rir::DeoptReason& v) const {
        return hash_combine(hash_combine(0, v.pc()), v.reason);
    }
};
} // namespace std

#endif
