#include "runtime/RuntimeFlags.h"
#include <cstdlib>
bool RuntimeFlags::contextualCompilationSkip = getenv("SKIP_CONTEXTUAL_COMPILATION") ? true : false;
