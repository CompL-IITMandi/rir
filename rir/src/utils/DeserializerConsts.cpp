#include "DeserializerConsts.h"
#include <cstdlib>

namespace rir {
    const char *DeserializerConsts::bitcodesPath = getenv("PIR_DESERIALIZE_PREFIX") ? getenv("PIR_DESERIALIZE_PREFIX") : "./bitcodes/";
    bool DeserializerConsts::earlyBitcodes = getenv("EARLY_BITCODES") ? getenv("EARLY_BITCODES")[0] == '1' : false;
    bool DeserializerConsts::bitcodesLoaded = false;
    bool DeserializerConsts::skipLLVMPasses = false;
}
