#ifndef RIR_COMPILER_H
#define RIR_COMPILER_H

#include "R/Preserve.h"
#include "R/Protect.h"
#include "R/r.h"
#include "runtime/DispatchTable.h"
#include "utils/FunctionWriter.h"
#include "utils/Pool.h"
#include "utils/UMap.h"

#include "api.h"

#include <unordered_map>
#include <iostream>
#include <functional>
#include <cassert>

typedef struct RCNTXT RCNTXT;
extern "C" SEXP R_syscall(int n, RCNTXT *cptr);
extern "C" SEXP R_sysfunction(int n, RCNTXT *cptr);

namespace rir {

static size_t createHastEntryForCodeObj(DispatchTable* vtable) {
    auto rirBody = vtable->baseline()->body();
    SEXP ast_1 = src_pool_at(globalContext(), rirBody->src);

    size_t hast = 0;
    hash_ast(ast_1, hast);

    if (hast == 0) {
        rirBody->disassemble(std::cout);
        std::cout << "TYPE: " << TYPEOF(ast_1) << std::endl;
        printAST(0, ast_1);
        // Rf_error("hast calculation failed");
    }

    rirBody->hast = hast;

    // There was a collision
    if (Code::hastCodeMap.find(hast) != Code::hastCodeMap.end()) {
        if (ast_1 == src_pool_at(globalContext(), ((rir::Code *)Code::hastCodeMap[hast])->src)) {
            std::cout << "BC recompilation for " << hast << std::endl;
            // rirBody->disassemble(std::cout);
            // ((rir::Code *)Code::hastCodeMap[hast])->disassemble(std::cout);
        } else {
            std::cout << "Existing in pool: " << hast << std::endl;
            printAST(0, src_pool_at(globalContext(), ((rir::Code *)Code::hastCodeMap[hast])->src));
            std::cout << "Trying to add: " << hast << std::endl;
            printAST(0, ast_1);
        }
    }

    Code::hastCodeMap[hast] = rirBody;
    return hast;
}

static size_t createHastEntryForClosure(DispatchTable* vtable, SEXP closure) {
    size_t hast = createHastEntryForCodeObj(vtable);
    Code::hastClosMap[hast] = closure;
    return hast;
}



class Compiler {
    SEXP exp;
    SEXP formals;
    SEXP closureEnv;

    Preserve preserve;

    explicit Compiler(SEXP exp)
        : exp(exp), formals(R_NilValue), closureEnv(nullptr) {
        preserve(exp);
    }

    Compiler(SEXP exp, SEXP formals, SEXP env)
        : exp(exp), formals(formals), closureEnv(env) {
        preserve(exp);
        preserve(formals);
        preserve(env);
    }

  public:
    static bool profile;
    static bool unsoundOpts;
    static bool loopPeelingEnabled;

    SEXP finalize();

    static SEXP compileExpression(SEXP ast) {
#if 0
        size_t count = 1;
        static std::unordered_map<SEXP, size_t> counts;
        if (counts.count(ast)) {
            counts.at(ast) = count = 1 + counts.at(ast);
        } else {
            counts[ast] = 1;
        }
        if (count % 200 == 0) {
            std::cout << "<<<<<<< Warning: expression compiled "
                      << count << "x:\n";
            Rf_PrintValue(ast);
            std::cout << "== Call:\n";
            Rf_PrintValue(R_syscall(0, R_GlobalContext));
            std::cout << "== Function:\n";
            Rf_PrintValue(R_sysfunction(0, R_GlobalContext));
            std::cout << ">>>>>>>\n";
        }
#endif

        Compiler c(ast);
        auto res = c.finalize();

        return res;
    }

    // To compile a function which is not yet closed
    static SEXP compileFunction(SEXP ast, SEXP formals) {
        Compiler c(ast, formals, nullptr);
        SEXP res = c.finalize();
        PROTECT(res);

        // Allocate a new vtable.
        DispatchTable* vtable = DispatchTable::create();

        // Initialize the vtable. Initially the table has one entry, which is
        // the compiled function.
        vtable->baseline(Function::unpack(res));

        // Set the closure fields.
        UNPROTECT(1);

        size_t hast = createHastEntryForCodeObj(vtable);

        int index = 0;
        SEXP map = Pool::get(1);
        if (map == R_NilValue) {
            UMap::createMapInCp(1);
            map = Pool::get(1);
        }
        vtable->baseline()->body()->populateSrcData(hast, map, true, index);

        return vtable->container();
    }

    static void compileClosure(SEXP inClosure) {

        assert(TYPEOF(inClosure) == CLOSXP);

        Protect p;

        SEXP body = BODY(inClosure);
        SEXP origBC = nullptr;
        if (TYPEOF(body) == BCODESXP) {
            origBC = p(body);
            body = VECTOR_ELT(CDR(body), 0);
        }

        Compiler c(body, FORMALS(inClosure), CLOENV(inClosure));
        SEXP compiledFun = p(c.finalize());

        // Allocate a new vtable.
        DispatchTable* vtable = DispatchTable::create();
        p(vtable->container());

        // Initialize the vtable. Initially the table has one entry, which is
        // the compiled function.
        vtable->baseline(Function::unpack(compiledFun));
        // Keep alive. TODO: why is this needed?
        if (origBC)
            vtable->baseline()->body()->addExtraPoolEntry(origBC);

        // Set the closure fields.
        SET_BODY(inClosure, vtable->container());

        size_t hast = createHastEntryForClosure(vtable, inClosure);

        int index = 0;
        SEXP map = Pool::get(1);
        if (map == R_NilValue) {
            UMap::createMapInCp(1);
            map = Pool::get(1);
        }
        vtable->baseline()->body()->populateSrcData(hast, map, true, index);
    }
};

}

#endif
