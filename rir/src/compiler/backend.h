#pragma once

#include "R/Preserve.h"
#include "compiler/log/debug.h"
#include "compiler/log/stream_logger.h"
#include "compiler/native/pir_jit_llvm.h"
#include "compiler/pir/module.h"
#include "compiler/pir/pir.h"
#include "runtime/Function.h"
#include "api.h"
#include "utils/serializerData.h"

#include <sstream>
#include <unordered_set>
#include <set>

namespace rir {
namespace pir {

class Backend {
  public:
    Backend(Module* m, StreamLogger& logger, const std::string& name)
        : module(m), jit(name), logger(logger) {

        }
    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;

    rir::Function* getOrCompile(ClosureVersion* cls);

    SEXP cData = nullptr;
    bool* serializerError = nullptr;

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
    StreamLogger& logger;

    rir::Function* doCompile(ClosureVersion* cls, ClosureStreamLogger& log);
};

} // namespace pir
} // namespace rir
