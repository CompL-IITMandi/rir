#pragma once

#include "R/Preserve.h"
#include "compiler/log/debug.h"
#include "compiler/log/loggers.h"
#include "compiler/native/pir_jit_llvm.h"
#include "compiler/pir/module.h"
#include "compiler/pir/pir.h"
#include "runtime/Function.h"

#include <sstream>
#include <unordered_set>

namespace rir {
namespace pir {

class Backend {
  public:
    Backend(Module* m, Log& logger, const std::string& name)
        : module(m), jit(name), logger(logger) {}
    ~Backend() { jit.finalize(); }
    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;

    rir::Function* getOrCompile(ClosureVersion* cls);

    void initSerializer(SEXP cData, bool* err) {
      assert(cData != nullptr);
      assert(*err == false);
      contextDataContainer = cData;
      serializerError = err;
    }

    bool serializerEnabled() {
      return (contextDataContainer != nullptr);
    }

    SEXP getCData() {
      return contextDataContainer;
    }

    bool* getSerializerError() {
      return serializerError;
    }

  private:
    struct LastDestructor {
        LastDestructor();
        ~LastDestructor();
    };
    LastDestructor firstMember_;
    Preserve preserve;

    Module* module;
    PirJitLLVM jit;
    std::unordered_map<ClosureVersion*, Function*> done;
    Log& logger;

    SEXP contextDataContainer = nullptr;
    bool* serializerError = nullptr;

    rir::Function* doCompile(ClosureVersion* cls, ClosureLog& log);
};

} // namespace pir
} // namespace rir
