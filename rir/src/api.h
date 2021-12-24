#ifndef API_H_
#define API_H_

#include "R/r.h"
#include "compiler/log/debug.h"
#include "runtime/Context.h"
#include "runtime/Function.h"

#include <stdint.h>
#include <set>

#define REXPORT extern "C"

extern int R_ENABLE_JIT;

struct FunctionMeta {
  std::unordered_map<unsigned long, std::vector<size_t>> dependencies; // context to dependency vector
  std::unordered_map<unsigned long, rir::Function *> functions; // context to function binary
};

class DeserializerData {
  public:
  // static std::unordered_map<size_t, FunctionMeta> binaryDependencyMap; //

  // static std::unordered_map<size_t, std::set<size_t>> hastUnlockMap; // try to free up these hasts when the key hast is freed

  // static std::unordered_map<size_t, void *> vtableMap; // pointer to vtable of a compiled hast
};

REXPORT SEXP rirInvocationCount(SEXP what);
REXPORT SEXP pirCompileWrapper(SEXP closure, SEXP name, SEXP debugFlags,
                               SEXP debugStyle);
REXPORT SEXP rirCompile(SEXP what, SEXP env);
REXPORT SEXP pirTests();
REXPORT SEXP pirCheck(SEXP f, SEXP check, SEXP env);
REXPORT SEXP pirSetDebugFlags(SEXP debugFlags);
SEXP pirCompile(SEXP closure, const rir::Context& assumptions,
                const std::string& name, const rir::pir::DebugOptions& debug);
extern SEXP rirOptDefaultOpts(SEXP closure, const rir::Context&, SEXP name);
extern SEXP rirOptDefaultOptsDryrun(SEXP closure, const rir::Context&,
                                    SEXP name);

void hash_ast(SEXP ast, size_t & hast);
void printAST(int space, SEXP ast);
void printAST(int space, int val);


REXPORT SEXP rirSerialize(SEXP data, SEXP file);
REXPORT SEXP rirDeserialize(SEXP file);

REXPORT SEXP rirSetUserContext(SEXP f, SEXP udc);
REXPORT SEXP rirCreateSimpleIntContext();


#endif // API_H_
