#ifndef RIR_COMPILER_H
#define RIR_COMPILER_H

#include "R/Preserve.h"
#include "R/Protect.h"
#include "R/r.h"
#include "runtime/DispatchTable.h"
#include "utils/FunctionWriter.h"
#include "utils/Pool.h"

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

        auto rirBody = vtable->baseline()->body();

        SEXP ast_1 = src_pool_at(globalContext(), rirBody->src);

        // The hast is inserted into the code object of the bytecode
        // Calculate hast for the given function
        int hast = 0;
        hash_ast(ast_1, hast);

        Code::hastMap[hast] = vtable;

        if (Code::cpHastPatch.count(hast) > 0) {
            auto cpEntry = cp_pool_at(globalContext(), Code::cpHastPatch[hast]);
            // SET_CLOENV(cpEntry, CLOENV(inClosure));
            SET_BODY(cpEntry, BODY(ast));
            SET_FORMALS(cpEntry, FORMALS(formals));
        }

        for (auto & meta : DeserializerData::deserializedHastMap[hast]) {
            FunctionWriter function;
            Preserve preserve;
            for (size_t i = 0; i < meta.fs.numArguments; ++i) {
                function.addArgWithoutDefault();
            }

            auto res = rir::Code::New(vtable->baseline()->body()->src);
            preserve(res->container());

            function.finalize(res, meta.fs, meta.c);

            function.function()->inheritFlags(vtable->baseline());
            vtable->insert(function.function());

            res->lazyCodeHandle(meta.nativeHandle);

            // insert the promises into the extra pools recursively
            std::function<void(std::string, rir::Code*)> updateExtrasRecursively = [&](std::string startingPrefix, rir::Code * code) {
                int i = 0;
                while (true) {
                    std::stringstream pName;
                    pName << startingPrefix << "_" << i;
                    if (std::count(meta.existingDefs.begin(), meta.existingDefs.end(), pName.str())) {
                        // This handle does exists inside for the code, so add this
                        // to the code objects extraPool
                        auto p = rir::Code::New(vtable->baseline()->body()->src);
                        preserve(p->container());
                        // Add the handle to the promise
                        p->lazyCodeHandle(pName.str());

                        #if COMPILER_PRINT_PORMISE_LINK == 1
                        std::cout << "Adding " << pName.str() << " to " << startingPrefix << std::endl;
                        #endif

                        // Add this created promise into the code's extra pool entry
                        code->addExtraPoolEntry(p->container());

                        // Check if there are any promises, inside this promise
                        updateExtrasRecursively(pName.str(), p);
                        // Process next promises
                        i++;
                        continue;
                    }
                    break;
                }
            };

            updateExtrasRecursively(meta.nativeHandle, res);

            // std::cout << "extraPoolIndices: " << meta.extraPoolIndices.size() << std::endl;

            for (auto & id : meta.extraPoolIndices) {
                auto ele = Pool::get(id);
                // DeoptMetadata * deoptMeta = static_cast<DeoptMetadata *>(DATAPTR(ele));
                // std::cout << "deoptMetadata: " << deoptMeta->numFrames << std::endl;
                res->addExtraPoolEntry(ele);
            }

            // for (int i = 0; i < res->extraPoolSize; i++) {
            //     std::cout << "extraPool: " << TYPEOF(res->getExtraPoolEntry(i)) << std::endl;
            // }
        }

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

        // int h1 = 0;
        // hash_ast(body, h1);

        // if (Code::hastMap.count(h1) > 0) {
        //     auto orig = src_pool_at(globalContext(), ((DispatchTable*)Code::hastMap[h1])->baseline()->body()->src);
        //     // already compiled, but duplicate
        //     SET_CLOENV(inClosure, CLOENV(orig) );

        //     SET_BODY(inClosure, BODY(orig));

        //     SET_FORMALS(inClosure, FORMALS(orig) );
        // }

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

        auto rirBody = vtable->baseline()->body();

        SEXP ast_1 = src_pool_at(globalContext(), rirBody->src);

        // The hast is inserted into the code object of the bytecode
        // Calculate hast for the given function
        int hast = 0;
        hash_ast(ast_1, hast);

        Code::hastMap[hast] = vtable;

        if (Code::cpHastPatch.count(hast) > 0) {
            // std::cout << "patching cp Entry" << std::endl;
            // std::cout << "CLOENV: " << CLOENV(inClosure) << std::endl;
            // std::cout << "FORMALS: " << FORMALS(inClosure) << std::endl;
            // std::cout << "BODY_EXPR: " << BODY_EXPR(inClosure) << std::endl;
            Pool::patch(Code::cpHastPatch[hast], inClosure);
            // auto cpEntry = cp_pool_at(globalContext(), Code::cpHastPatch[hast]);
            // SET_CLOENV(cpEntry, CLOENV(inClosure));
            // SET_BODY(cpEntry, BODY(inClosure));
            // SET_FORMALS(cpEntry, FORMALS(inClosure));
        }

        for (auto & meta : DeserializerData::deserializedHastMap[hast]) {
            FunctionWriter function;
            Preserve preserve;
            for (size_t i = 0; i < meta.fs.numArguments; ++i) {
                function.addArgWithoutDefault();
            }

            auto res = rir::Code::New(vtable->baseline()->body()->src);
            preserve(res->container());

            function.finalize(res, meta.fs, meta.c);

            function.function()->inheritFlags(vtable->baseline());
            vtable->insert(function.function());

            res->lazyCodeHandle(meta.nativeHandle);

            // ExtraIndex
            int eIndex = 0;

            // insert the promises into the extra pools recursively
            std::function<void(std::string, rir::Code*)> updateExtrasRecursively = [&](std::string startingPrefix, rir::Code * code) {
                int i = 0;
                while (true) {
                    std::stringstream pName;
                    pName << startingPrefix << "_" << i;
                    if (std::count(meta.existingDefs.begin(), meta.existingDefs.end(), pName.str())) {
                        // Store the srcIndex of the resolved data here, instead of runtime data (maybe improve?)
                        unsigned patchedSrcIdx = meta.promiseSrcEntries[eIndex++];

                        // This handle does exists inside for the code, so add this
                        // to the code objects extraPool
                        auto p = rir::Code::New(patchedSrcIdx);
                        preserve(p->container());
                        // Add the handle to the promise
                        p->lazyCodeHandle(pName.str());

                        #if COMPILER_PRINT_PORMISE_LINK == 1
                        std::cout << "Adding " << pName.str() << " to " << startingPrefix << std::endl;
                        #endif


                        // Add this created promise into the code's extra pool entry
                        code->addExtraPoolEntry(p->container());

                        // Check if there are any promises, inside this promise
                        updateExtrasRecursively(pName.str(), p);
                        // Process next promises
                        i++;
                        continue;
                    }
                    break;
                }
            };

            updateExtrasRecursively(meta.nativeHandle, res);

            // std::cout << "extraPoolIndices: " << meta.extraPoolIndices.size() << std::endl;

            for (auto & id : meta.extraPoolIndices) {
                auto ele = Pool::get(id);
                // DeoptMetadata * deoptMeta = static_cast<DeoptMetadata *>(DATAPTR(ele));
                // std::cout << "deoptMetadata: " << deoptMeta->numFrames << std::endl;
                res->addExtraPoolEntry(ele);
            }

            // for (int i = 0; i < res->extraPoolSize; i++) {
            //     std::cout << "extraPool: " << TYPEOF(res->getExtraPoolEntry(i)) << std::endl;
            // }
        }
    }
};

}

#endif
