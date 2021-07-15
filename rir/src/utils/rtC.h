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
      std::string & funName,
      size_t & key,
      double & runtime,
      Context & original_intention,
      bool & tragedy,
      bool & deopt
    );

    static void saveCompilationData(
      SEXP callee,
      Context compiled_context,
      double time,
      int bb,
      int p
    );

    static void registerFailedCompilation(SEXP callee);

    static void saveFunctionMap(
        std::unordered_map<std::string, bool>
    );
};

} // namespace rir

#endif
