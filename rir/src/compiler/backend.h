#pragma once

#include "R/Preserve.h"
#include "compiler/log/debug.h"
#include "compiler/log/stream_logger.h"
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
    Backend(Module* m, StreamLogger& logger, const std::string& name)
        : module(m), jit(name), logger(logger) {
          sCallback = NULL;
        }
    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;

    rir::Function* getOrCompile(ClosureVersion* cls);

    void addSerializer(std::function<void(llvm::Module*)>, std::function<void(FunctionSignature &)>);

  private:
    struct LastDestructor {
        LastDestructor();
        ~LastDestructor();
    };
    LastDestructor firstMember_;
    Preserve preserve;
    std::function<void(llvm::Module*)> sCallback;
    std::function<void(FunctionSignature &)> signatureCallback;
    Module* module;
    PirJitLLVM jit;
    std::unordered_map<ClosureVersion*, Function*> done;
    StreamLogger& logger;

    rir::Function* doCompile(ClosureVersion* cls, ClosureStreamLogger& log);
};

} // namespace pir
} // namespace rir
