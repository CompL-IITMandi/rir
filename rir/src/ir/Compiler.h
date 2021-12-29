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

#include "patches.h"

#include <unordered_map>
#include <iostream>
#include <functional>
#include <cassert>

typedef struct RCNTXT RCNTXT;
extern "C" SEXP R_syscall(int n, RCNTXT *cptr);
extern "C" SEXP R_sysfunction(int n, RCNTXT *cptr);

namespace rir {

static size_t getHast(DispatchTable* vtable) {
    auto rirBody = vtable->baseline()->body();
    SEXP ast_1 = src_pool_at(globalContext(), rirBody->src);

    size_t hast = 0;
    hash_ast(ast_1, hast);

    return hast;
}

static void insertVTable(DispatchTable* vtable, size_t hast) {
    SEXP map = Pool::get(3);
    if (map != R_NilValue) {
        SEXP hastSym = Rf_install(std::to_string(hast).c_str());
        UMap::insert(map, hastSym, vtable->container());
    }
}

static void insertClosObj(SEXP clos, size_t hast) {
    SEXP map = Pool::get(4);
    if (map != R_NilValue) {
        SEXP hastSym = Rf_install(std::to_string(hast).c_str());
        UMap::insert(map, hastSym, clos);
    }
}


static void tryLinking(DispatchTable* vtable, SEXP hSym) {
    SEXP serMap = Pool::get(1);
    if (serMap == R_NilValue) return;
    SEXP unlMap = Pool::get(2);
    SEXP tabMap = Pool::get(3);


    if (UMap::symbolExistsInMap(hSym, serMap)) {
        // std::cout << "symbolExistsInMap: " << CHAR(PRINTNAME(hSym)) << std::endl;
        SEXP hMap = DeserialDataMap::getHastEnv(serMap, hSym);
        SEXP contexts = VECTOR_ELT(hMap, 0);
        bool noMoreContexts = true;
        for (int i = 0; i < Rf_length(contexts); i++) {
            SEXP contextSym = VECTOR_ELT(contexts, i);
            if (contextSym == R_NilValue) continue;
            // std::cout << "contextSym: " << CHAR(PRINTNAME(contextSym)) << std::endl;
            SEXP vec = UMap::get(hMap, contextSym);
            bool dependenciesSatisfied = true;
            for (int j = 1; j < Rf_length(vec); j++) {
                SEXP depSym = VECTOR_ELT(vec, j);
                if (depSym == R_NilValue) continue;
                if (UMap::symbolExistsInMap(depSym, tabMap)) {
                    VecOpr::remove(vec, j);
                } else {
                    dependenciesSatisfied = false;
                }
            }

            if (dependenciesSatisfied) {
                SEXP func = VECTOR_ELT(vec, 0);
                Function * function = Function::unpack(func);
                function->inheritFlags(vtable->baseline());
                vtable->insert(function);

                // std::cout << "linking successful " << CHAR(PRINTNAME(contextSym)) << std::endl;

                UMap::remove(hMap, contextSym);
            } else {
                noMoreContexts = false;
            }
        }

        if (noMoreContexts) {
            // std::cout << "noMoreContexts for " << CHAR(PRINTNAME(hSym)) << std::endl;
            UMap::remove(serMap, hSym);
        }

        if (UMap::symbolExistsInMap(hSym, unlMap)) {
            SEXP unlockVec = UMap::get(unlMap, hSym);
            for (int i = 0; i < Rf_length(unlockVec); i++) {
                SEXP unlSym = VECTOR_ELT(unlockVec, i);
                if (UMap::symbolExistsInMap(unlSym, tabMap)) {
                    // std::cout << "trying to link symbol: " << CHAR(PRINTNAME(unlSym)) << std::endl;
                    SEXP tableContainer = UMap::get(tabMap, unlSym);
                    DispatchTable * vtable = DispatchTable::unpack(tableContainer);
                    tryLinking(vtable, unlSym);
                } else {
                    // std::cout << "cannot link symbol yet: " << CHAR(PRINTNAME(unlSym)) << std::endl ;
                }
            }
            UMap::remove(unlMap, hSym);
        }


    }
}

static void tryLinking(DispatchTable* vtable, size_t hast) {
    SEXP hastSym = Rf_install(std::to_string(hast).c_str());
    tryLinking(vtable, hastSym);
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

        // Rf_PrintValue(ast);
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

        UNPROTECT(1);
        size_t hast = getHast(vtable);

        insertVTable(vtable, hast);

        tryLinking(vtable, hast);

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

        size_t hast = getHast(vtable);

        insertVTable(vtable, hast);

        insertClosObj(inClosure, hast);

        tryLinking(vtable, hast);

    }
};

}

#endif
