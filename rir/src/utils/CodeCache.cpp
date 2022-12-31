#include "CodeCache.h"
#include <cstdlib>
namespace rir {
bool CodeCache::serializer = std::getenv("CC_SERIALIZER") ? std::getenv("CC_SERIALIZER")[0] == '1' : false;
// bool CodeCache::captureCompileStats = std::getenv("CAPTURE_ALL_COMPILE_STATS") ? true : false;
}
