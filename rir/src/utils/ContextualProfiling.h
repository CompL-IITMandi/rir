#ifndef CONTEXTUAL_PROFILE_H
#define CONTEXTUAL_PROFILE_H
#include "../interpreter/call_context.h"
#include <chrono>
#include <ctime>

namespace rir {
// CONTEXT_LOGS           - Enable Logging
// COMPILE_ONLY_ONCE      - Only compile once
// SKIP_ALL_COMPILATION   - Skip All Compilation
// OUT_NAME               - Name for the output

class ContextualProfiling {
  public:
    static bool compileFlag(
      size_t,
      Context
      );

    static void createCallEntry(
      CallContext const& // logs [ name, callType ]
      );
    static void recordCodePoint(
      int,
      std::string,
      std::string
    );
    // static void addContextData(
    //   CallContext&,
    //   std::string
    // );
    static size_t getEntryKey(
      CallContext const&
    );
    static void addFunctionDispatchRuntime(
      size_t,
      Context,
      Function const&,
      std::chrono::duration<double>
    );
    static void addFunctionDispatchInfo(
      size_t,
      Context,
      Function const&,
      SEXP
    );
    static void countSuccessfulCompilation(
      SEXP,
      Context,
      std::chrono::duration<double>,
      pir::ClosureVersion*,
      Function*,
      int sequence
    );
    static void pirBeforeCompilation(
      SEXP,
      Context,
      pir::ClosureVersion*
    );
    static void countFailedCompilation(
      SEXP,
      Context,
      std::chrono::duration<double>,
      int sequence
    );
};

} // namespace rir

#endif
