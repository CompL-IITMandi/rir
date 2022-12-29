#include "CodeCache.h"
#include <cstdlib>
namespace rir {
bool CodeCache::serializer = std::getenv("CC_SERIALIZER") ? true : false;
// bool CodeCache::captureCompileStats = std::getenv("CAPTURE_ALL_COMPILE_STATS") ? true : false;
}
