#ifndef RIR_DEOPTIMIZATION_H
#define RIR_DEOPTIMIZATION_H

#include <R/r.h>
#include <iostream>

namespace rir {
#pragma pack(push)
#pragma pack(1)

enum class Opcode : uint8_t;
struct Code;

struct FrameInfo {
    Opcode* pc;
    uintptr_t offset;
    Code* code;
    size_t hast;
    int index;
    size_t stackSize;
    bool inPromise;


    FrameInfo() {}
    FrameInfo(uintptr_t offset, size_t hast, int index, size_t stackSize, bool promise)
        : offset(offset), hast(hast), index(index), stackSize(stackSize), inPromise(promise) {
            code = 0;
            pc = 0;
        }
    FrameInfo(Opcode* pc, Code* code, size_t stackSize, bool promise)
        : pc(pc), code(code), stackSize(stackSize), inPromise(promise) {}
};

struct DeoptMetadata {
    void print(std::ostream& out) const;
    size_t numFrames;
    FrameInfo frames[];
};

#pragma pack(pop)
} // namespace rir

#endif
