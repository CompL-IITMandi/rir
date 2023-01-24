#pragma once
#include <string>
#include <unordered_map>

namespace rir {
    class DeserializerConsts {
    public:
        const static char *bitcodesPath;
        static bool earlyBitcodes;
        static bool bitcodesLoaded;
        static bool skipLLVMPasses;
        static std::unordered_map<size_t, unsigned int> serializedPools;
    };
}
