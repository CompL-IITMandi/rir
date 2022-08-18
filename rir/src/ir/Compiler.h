#ifndef RIR_COMPILER_H
#define RIR_COMPILER_H

#include "R/Preserve.h"
#include "R/Protect.h"
#include "R/r.h"
#include "runtime/DispatchTable.h"
#include "utils/FunctionWriter.h"
#include "utils/Pool.h"

#include <unordered_map>
#include <iostream>
#include <functional>
#include <cassert>

#include "runtimePatches.h"
#include "R/Printing.h"
#include "api.h"

#include "compiler/pir/module.h"
#include "compiler/log/stream_logger.h"
#include "compiler/compiler.h"
#include "compiler/backend.h"
#include "utils/BitcodeLinkUtility.h"
#include "utils/WorklistManager.h"

#define DEBUG_TABLE_ENTRIES 0
#define ONLY_APPLY_MASK 0
#include <chrono>
using namespace std::chrono;

typedef struct RCNTXT RCNTXT;
extern "C" SEXP R_syscall(int n, RCNTXT *cptr);
extern "C" SEXP R_sysfunction(int n, RCNTXT *cptr);

namespace rir {

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


        SEXP hast = BitcodeLinkUtil::getHast(body, CLOENV(inClosure));
        // Set the closure fields.
        SET_BODY(inClosure, vtable->container());

        // Only Serializer run
        static bool onlySerializerRun = getenv("ONLY_SERIALIZER_RUN") ? getenv("ONLY_SERIALIZER_RUN")[0] == '1' : false;

        // Only Deserializer Run
        static bool onlyDeserializerRun = getenv("ONLY_DESERIALIZER_RUN") ? getenv("ONLY_DESERIALIZER_RUN")[0] == '1' : false;

        if (hast != R_NilValue && BitcodeLinkUtil::readyForSerialization(inClosure, vtable, hast)) {
            #if DEBUG_TABLE_ENTRIES == 1
            std::cout << "(R) Hast: " << CHAR(PRINTNAME(hast)) << " (Adding table, closure and populating src Map): " << (uintptr_t)inClosure << std::endl;
            BitcodeLinkUtil::printSources(vtable, hast);
            #endif

            vtable->hast = hast;
            BitcodeLinkUtil::insertVTable(vtable, hast);
            BitcodeLinkUtil::insertClosObj(inClosure, hast);

            // Hast Src data is not needed in pure deserializer run
            if (onlySerializerRun) {
                BitcodeLinkUtil::populateHastSrcData(vtable, hast);
            } else if (onlyDeserializerRun) {
                #if ONLY_APPLY_MASK == 1
                BitcodeLinkUtil::applyMask(vtable, hast);
                #else
                BitcodeLinkUtil::tryLinking(vtable, hast);
                BitcodeLinkUtil::tryUnlocking(hast);
                #endif
            } else {
                BitcodeLinkUtil::populateHastSrcData(vtable, hast);

                // #if ONLY_APPLY_MASK == 1
                // BitcodeLinkUtil::applyMask(vtable, hast);
                // #else
                // #endif

                if (GeneralWorklist::get(hast)) {
                    // Bitcode is available for this hast, do worklist

                    // Tries to link available bitcodes, if they are not unlocked then adds them to either worklist1 or worklist2
                    BitcodeLinkUtil::tryLinking(vtable, hast);

                    // Remove entry from general worklist after work is complete
                    GeneralWorklist::remove(hast);
                } else {
                    // Non serialized code can also have work to do
                    // Do work on worklist1 (if work exists).
                    BitcodeLinkUtil::tryUnlocking(hast);
                }

            }
        } else {
            #if DEBUG_TABLE_ENTRIES == 1
            std::cout << "(BLACK) Hast: " << CHAR(PRINTNAME(hast)) << " (Adding table, closure and populating src Map): " << (uintptr_t)inClosure << std::endl;
            BitcodeLinkUtil::printSources(vtable, hast);
            #endif
        }
    }
};

}

#endif
