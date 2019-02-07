#ifndef PIR_ASSUMPTIONS_H
#define PIR_ASSUMPTIONS_H

#include "utils/EnumSet.h"

#include <array>
#include <cstring>
#include <iostream>

namespace rir {

enum class Assumption {
    EagerArgs_, // All arguments are already evaluated

    Arg1IsEager_, // Arg1 is already evaluated
    Arg2IsEager_, // Arg2 is already evaluated
    Arg3IsEager_, // Arg3 is already evaluated

    NonObjectArgs_, // All arguments are not objects

    Arg1IsNonObj_, // Arg1 is not an object
    Arg2IsNonObj_, // Arg2 is not an object
    Arg3IsNonObj_, // Arg3 is not an object

    NoExplicitlyMissingArgs, // Explicitly missing, e.g. f(,,)
    CorrectOrderOfArguments, // Ie. the args are not named
    NotTooFewArguments,      // The number of args supplied is as expected, ie.
                             //  supplied >= (nargs - missing)
                             //  Note: can still have explicitly missing args
    NotTooManyArguments,     // The number of args supplied is <= nargs

    FIRST = EagerArgs_,
    LAST = NotTooManyArguments
};

#pragma pack(push)
#pragma pack(1)
struct Assumptions {
    typedef EnumSet<Assumption, uint16_t> Flags;

    Assumptions() = default;
    Assumptions(const Assumptions&) noexcept = default;

    explicit constexpr Assumptions(const Flags& flags) : flags(flags) {}
    constexpr Assumptions(const Flags& flags, uint8_t missing)
        : flags(flags), missing(missing) {}
    explicit Assumptions(uint32_t i) {
        // Silences unused warning:
        (void)unused;
        memcpy(this, &i, sizeof(i));
    }

    constexpr static size_t MAX_MISSING = 255;

    RIR_INLINE void add(Assumption a) { flags.set(a); }
    RIR_INLINE void remove(Assumption a) { flags.reset(a); }
    RIR_INLINE bool includes(Assumption a) const { return flags.includes(a); }
    RIR_INLINE bool includes(const Flags& a) const { return flags.includes(a); }

    RIR_INLINE bool isEager(size_t i) const;
    RIR_INLINE void setEager(size_t i, bool);
    RIR_INLINE bool notObj(size_t i) const;
    RIR_INLINE void setNotObj(size_t i, bool);

    RIR_INLINE uint8_t numMissing() const { return missing; }

    RIR_INLINE void numMissing(long i) {
        assert(i < 255);
        missing = i;
    }

    RIR_INLINE bool empty() const { return flags.empty() && missing == 0; }

    RIR_INLINE size_t count() const { return flags.count(); }

    constexpr Assumptions operator|(const Assumptions& other) const {
        assert(missing == other.missing);
        return Assumptions(other.flags | flags, missing);
    }

    RIR_INLINE bool operator<(const Assumptions& other) const {
        // Order by number of assumptions! Important for dispatching.
        if (missing != other.missing)
            return missing > other.missing;
        if (flags.count() != other.flags.count())
            return flags.count() > other.flags.count();
        return flags < other.flags;
    }

    RIR_INLINE bool operator!=(const Assumptions& other) const {
        return flags != other.flags || missing != other.missing;
    }

    RIR_INLINE bool operator==(const Assumptions& other) const {
        return flags == other.flags && missing == other.missing;
    }

    RIR_INLINE bool subtype(const Assumptions& other) const {
        if (other.flags.includes(Assumption::NotTooFewArguments))
            return missing == other.missing && other.flags.includes(flags);
        return other.flags.includes(flags);
    }

    friend struct std::hash<rir::Assumptions>;
    friend std::ostream& operator<<(std::ostream& out, const Assumptions& a);

    static constexpr std::array<Assumption, 3> ObjAssumptions = {
        {Assumption::Arg1IsNonObj_, Assumption::Arg2IsNonObj_,
         Assumption::Arg3IsNonObj_}};
    static constexpr std::array<Assumption, 3> EagerAssumptions = {
        {Assumption::Arg1IsEager_, Assumption::Arg2IsEager_,
         Assumption::Arg3IsEager_}};

  private:
    Flags flags;
    uint8_t missing = 0;
    uint8_t unused = 0;
};
#pragma pack(pop)

RIR_INLINE bool Assumptions::isEager(size_t i) const {
    if (flags.includes(Assumption::EagerArgs_))
        return true;

    if (i < EagerAssumptions.size())
        if (flags.includes(EagerAssumptions[i]))
            return true;

    return false;
}

RIR_INLINE void Assumptions::setEager(size_t i, bool eager) {
    if (eager && i < EagerAssumptions.size())
        flags.set(EagerAssumptions[i]);
    else if (!eager)
        flags.reset(Assumption::EagerArgs_);
}

RIR_INLINE bool Assumptions::notObj(size_t i) const {
    if (flags.includes(Assumption::NonObjectArgs_))
        return true;

    if (i < ObjAssumptions.size())
        if (flags.includes(ObjAssumptions[i]))
            return true;

    return false;
}

RIR_INLINE void Assumptions::setNotObj(size_t i, bool notObj) {
    if (notObj && i < ObjAssumptions.size())
        flags.set(ObjAssumptions[i]);
    else if (!notObj)
        flags.reset(Assumption::NonObjectArgs_);
}

typedef uint32_t Immediate;
static_assert(sizeof(Assumptions) == sizeof(Immediate),
              "Assumptions needs to be one immediate arg size");

std::ostream& operator<<(std::ostream& out, Assumption a);

} // namespace rir

namespace std {
template <>
struct hash<rir::Assumptions> {
    std::size_t operator()(const rir::Assumptions& v) const {
        return hash_combine(hash_combine(0, v.flags.to_i()), v.missing);
    }
};
} // namespace std

#endif
