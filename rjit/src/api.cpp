#include <llvm/IR/Module.h>

#include "Compiler.h"

#include "api.h"

#include "RIntlns.h"


#include "ir/ir.h"
#include "ir/Builder.h"
#include "ir/intrinsics.h"
#include "ir/Handler.h"

using namespace rjit;

/** Compiles given ast and returns the NATIVESXP for it.
 */
REXPORT SEXP jitAst(SEXP ast) {
    Compiler c("module");
    SEXP result = c.compile("rfunction", ast);
    c.jitAll();
    return result;
}

/** More complex compilation method that compiles multiple functions into a
  specified module name.

  The module name is expected to be a STRSXP and the functions is expected to be
  a pairlist. If pairlist has tags associated with the elements, they will be
  used as function names.
 */
REXPORT SEXP jitFunctions(SEXP moduleName, SEXP functions) {
    char const* mName = CHAR(STRING_ELT(moduleName, 0));
    Compiler c(mName);
    while (functions != R_NilValue) {
        SEXP f = CAR(functions);
        // get the function ast
        SEXP body = BODY(f);
        SEXP name = TAG(functions);
        char const* fName =
            (name == R_NilValue) ? "unnamed function" : CHAR(PRINTNAME(name));
        if (TYPEOF(body) == BCODESXP)
            warning("Ignoring %s because it is in bytecode", fName);
        else if (TYPEOF(body) == NATIVESXP)
            warning("Ignoring %s because it is already compiled", fName);
        else
            SET_BODY(f, c.compileFunction(fName, body));
        // move to next function
        functions = CDR(functions);
    }
    c.jitAll();
    return moduleName;
}

/** Returns the constant pool associated with the given NATIVESXP.
 */
REXPORT SEXP jitConstants(SEXP expression) {
    assert(TYPEOF(expression) == NATIVESXP and
           "JIT constants can only be extracted from a NATIVESXP argument");
    return CDR(expression);
}

/** Displays the LLVM IR for given NATIVESXP.
 */
REXPORT SEXP jitLLVM(SEXP expression) {
    assert(TYPEOF(expression) == NATIVESXP and
           "LLVM code can only be extracted from a NATIVESXP argument");
    llvm::Function* f = reinterpret_cast<llvm::Function*>(TAG(expression));
    f->dump();
    return R_NilValue;
}



using namespace llvm;

REXPORT SEXP jittest(SEXP expression) {
    ir::Builder b("someName");
    b.openFunction("f1", expression, false);
    llvm::Value * lhs = ir::GenericGetVar::create(b, CAR(CDR(expression)), b.rho());
    llvm::Value * rhs = ir::GenericGetVar::create(b, CAR(CDR(CDR(expression))), b.rho());
    llvm::Value * ret = ir::GenericSub::create(b, lhs, rhs, expression, b.rho());
    llvm::ReturnInst::Create(llvm::getGlobalContext(), ret, b);
    static_cast<llvm::Function*>(b)->dump();

    BasicBlock::iterator i= b.block()->begin();
    ir::MyHandler mh;
    while (i != b.block()->end()) {
        mh.dispatch(i);
    }
    return expression;
}


