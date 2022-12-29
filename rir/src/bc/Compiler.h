#ifndef RIR_COMPILER_H
#define RIR_COMPILER_H

#include "R/Preserve.h"
#include "R/Protect.h"
#include "R/r.h"
#include "runtime/DispatchTable.h"
#include "utils/FunctionWriter.h"
#include "utils/Pool.h"
#include "utils/Hast.h"

#include <cassert>
#include <functional>
#include <iostream>
#include <unordered_map>

#define DEBUG_TABLE_ENTRIES 0
#define DEBUG_BLACKLIST 0

namespace rir {

class Compiler {
  private:
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

    SEXP finalize();

  public:
    static bool profile;
    static bool unsoundOpts;
    static bool loopPeelingEnabled;

    static SEXP compileExpression(SEXP ast) {
        Compiler c(ast);
        return c.finalize();
    }

    // Compile a function which is not yet closed
    static SEXP compileFunction(SEXP ast, SEXP formals) {
        Protect p;

        Compiler c(ast, formals, nullptr);
        auto res = p(c.finalize());

        // Allocate a new vtable.
        auto dt = DispatchTable::create();

        // Initialize the vtable. Initially the table has one entry, which is
        // the compiled function.
        dt->baseline(Function::unpack(res));

        return dt->container();
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

        // Calculate hast
        SEXP hast = Hast::getHast(body, CLOENV(inClosure));
        Compiler c(body, FORMALS(inClosure), CLOENV(inClosure));
        auto res = p(c.finalize());

        // Allocate a new vtable.
        auto dt = DispatchTable::create();
        p(dt->container());

        // Initialize the vtable. Initially the table has one entry, which is
        // the compiled function.
        dt->baseline(Function::unpack(res));
        // Keep alive. TODO: why is this needed?
        if (origBC)
            dt->baseline()->body()->addExtraPoolEntry(origBC);

        // Set the closure fields.
        SET_BODY(inClosure, dt->container());

        if (hast != R_NilValue) {
            // Check if the hast is already seen
            if (Hast::hastMap.count(hast) > 0) {
                #if DEBUG_BLACKLIST == 1 || DEBUG_TABLE_ENTRIES == 1
                std::cout << "[Blacklisting] Duplicate Hast: " << CHAR(PRINTNAME(hast)) << std::endl;
                #if DEBUG_BLACKLIST == 1
                Rf_PrintValue(inClosure);
                #endif
                #endif
                return;
            }

            #if DEBUG_TABLE_ENTRIES == 1
            std::cout << "[Hasting] Adding Hast: " << CHAR(PRINTNAME(hast)) << std::endl;
            // Rf_PrintValue(inClosure);
            #endif

            // Add the current object to map
            Hast::hastMap[hast] = {dt->container(), inClosure};

            // vtable->hast = hast;

            // Hast Src data is not needed in pure deserializer run
            Hast::populateHastSrcData(dt, hast);

            // if (GeneralWorklist::get(hast)) {
            //     // Bitcode is available for this hast, do worklist

            //     // Tries to link available bitcodes, if they are not unlocked then adds them to either worklist1 or worklist2
            //     BitcodeLinkUtil::tryLinking(vtable, hast);

            //     // Remove entry from general worklist after work is complete
            //     GeneralWorklist::remove(hast);
            // } else {
            //     // Non serialized code can also have work to do
            //     // Do work on worklist1 (if work exists).
            //     BitcodeLinkUtil::tryUnlocking(hast);
            // }

        } else {
            #if DEBUG_TABLE_ENTRIES == 1
            std::cout << "[Blacklisting] Invalid Hast (Anon: " << (Hast::isAnonEnv(CLOENV(inClosure)) ? "True" : "False") << ")" <<  std::endl;
            #endif
        }
    }
};

} // namespace rir

#endif
