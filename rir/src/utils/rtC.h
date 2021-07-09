#ifndef RT_C__H
#define RT_C__H
#include "../interpreter/call_context.h"
#include <chrono>
#include <ctime>

namespace rir {

// RT_LOG="filename"
// SAVE_BIN="fileName"
// LOAD_BIN="fileName"

class rtC {
  public:
    static void createEntry(
      CallContext &
    );

    static void addDispatchInfo(
        std::string name,
        size_t call,
        Context dispatched_context,
        bool baseline,
        bool deopt,
        double runtime,
        bool tragedy,
        Context original_intention
    );

    static void saveCompilationData(
        SEXP callee,
        Context compiled_context,
        double time,
        int bb,
        int p
    );

    static void registerFailedCompilation();

    static void saveFunctionMap(
        std::unordered_map<std::string, bool>
    );
};

} // namespace rir

#endif
