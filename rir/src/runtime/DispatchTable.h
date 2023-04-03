#ifndef RIR_DISPATCH_TABLE_H
#define RIR_DISPATCH_TABLE_H

#include "Function.h"
#include "R/Serialize.h"
#include "RirRuntimeObject.h"
#include "utils/random.h"
#include "runtime/L2Dispatch.h"

namespace rir {

#define DISPATCH_TABLE_MAGIC (unsigned)0xd7ab1e00

typedef SEXP DispatchTableEntry;

/*
 * A dispatch table (vtable) for functions.
 *
 */
#pragma pack(push)
#pragma pack(1)
struct DispatchTable
    : public RirRuntimeObject<DispatchTable, DISPATCH_TABLE_MAGIC> {

    size_t size() const { return size_; }


    Function* best() const {
        assert(false && "--- L2 BROKEN FEATURE ---");
        // BROKEN
        // if (size() > 1) {
        //     return get(1);
        // }

        return baseline();
    }
    Function* baseline() const {
        auto f = Function::unpack(getEntry(0));
        assert(f->signature().envCreation ==
               FunctionSignature::Environment::CallerProvided);
        return f;
    }

    inline Function* dispatch(Context a, bool ignorePending = true) const {
        return dispatchConsideringDisabled(a, nullptr, ignorePending);
    }

    Function* dispatchConsideringDisabled(Context a, Function** disabledFunc,
                                          bool ignorePending = true) const;

    void baseline(Function* f) {
        assert(f->signature().optimization ==
               FunctionSignature::OptimizationLevel::Baseline);
        if (size() == 0)
            size_++;
        else
            assert(baseline()->signature().optimization ==
                   FunctionSignature::OptimizationLevel::Baseline);
        setEntry(0, f->container());
    }

    bool containsL1Context(const Context& assumptions) const {
        for (size_t i = 1; i < size(); ++i) {
            // auto eCon = getContext(i);
            auto funContainer = getEntry(i);
            Context currContext;

            if (L2Dispatch::check(funContainer)) {
                L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
                currContext = l2vt->getContext();
            } else {
                assert(Function::check(funContainer));
                currContext = Function::unpack(funContainer)->context();
            }

            if (currContext == assumptions) {
                return true;
            }
        }
        return false;
    }

    bool containsDispatchableL1(const Context& assumptions) const {
        for (size_t i = 1; i < size(); ++i) {
            // auto eCon = getContext(i);
            auto funContainer = getEntry(i);
            Context currContext;

            if (L2Dispatch::check(funContainer)) {
                L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
                currContext = l2vt->getContext();
            } else {
                assert(Function::check(funContainer));
                currContext = Function::unpack(funContainer)->context();
            }

            if (currContext == assumptions) {
                Function * currFun;
                if (L2Dispatch::check(funContainer)) {
                    L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
                    currFun = l2vt->dispatch();
                } else {
                    assert(Function::check(funContainer));
                    currFun = Function::unpack(funContainer);
                }

                if (currFun) {
                    return !currFun->disabled();
                }
                return false;
            }
        }
        return false;
    }

    void remove(Code* funCode) {
        size_t i = 1;
        for (; i < size(); ++i) {
            SEXP funContainer = getEntry(i);
            Function * currFun;
            if (L2Dispatch::check(funContainer)) {
                L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
                currFun = l2vt->dispatch();
            } else {
                assert(Function::check(funContainer));
                currFun = Function::unpack(funContainer);
            }
            if (currFun->body() == funCode)
                break;
        }
        if (i == size())
            return;
        for (; i < size() - 1; ++i) {
            setEntry(i, getEntry(i + 1));
        }
        SEXP funContainer = getEntry(i);
        if (L2Dispatch::check(funContainer)) {
            L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
            l2vt->setFallback(R_NilValue);
        } else {
            setEntry(i, nullptr);
        }
        size_--;
    }


    void insert(Function* fun);

    void insertL2(Function* fun);

    // void insertL2V1(Function* fun);

    // Function slot negotiation
    int negotiateSlot(const Context& assumptions);

    void print(std::ostream& out, const int & space = 0) const;

    static DispatchTable* create(size_t capacity = 20) {
        size_t sz =
            sizeof(DispatchTable) + (capacity * sizeof(DispatchTableEntry));
        SEXP s = Rf_allocVector(EXTERNALSXP, sz);
        return new (INTEGER(s)) DispatchTable(capacity);
    }

    size_t capacity() const { return info.gc_area_length; }

    static DispatchTable* deserialize(SEXP refTable, R_inpstream_t inp) {
        DispatchTable* table = create();
        PROTECT(table->container());
        AddReadRef(refTable, table->container());
        table->size_ = InInteger(inp);
        for (size_t i = 0; i < table->size(); i++) {
            table->setEntry(i,
                            Function::deserialize(refTable, inp)->container());
        }
        UNPROTECT(1);
        return table;
    }

    void serialize(SEXP refTable, R_outpstream_t out) const {
        HashAdd(container(), refTable);
        OutInteger(out, 1);
        baseline()->serialize(refTable, out);
    }

    Context userDefinedContext() const { return userDefinedContext_; }
    DispatchTable* newWithUserContext(Context udc) {

        auto clone = create(this->capacity());
        clone->setEntry(0, this->getEntry(0));
        assert(false && "--- L2 BROKEN FEATURE ---");
        // BROKEN
        // auto j = 1;
        // for (size_t i = 1; i < size(); i++) {
        //     if (get(i)->context().smaller(udc)) {
        //         clone->setEntry(j, getEntry(i));
        //         j++;
        //     }
        // }

        // clone->size_ = j;
        // clone->userDefinedContext_ = udc;
        return clone;
    }

    Context combineContextWith(Context anotherContext) {
        return userDefinedContext_ | anotherContext;
    }

    unsigned jitLag = 4;
    unsigned jitTick = 4;
    size_t lastCompilationState = SIZE_MAX;

    SEXP hast = nullptr;
    int offsetIdx = -1;
    SEXP tmpCallee = nullptr; // only used for logging purposes!!

  private:
    DispatchTable() = delete;
    explicit DispatchTable(size_t cap)
        : RirRuntimeObject(
              // GC area starts at the end of the DispatchTable
              sizeof(DispatchTable),
              // GC area is just the pointers in the entry array
              cap) {}

    size_t size_ = 0;
    Context userDefinedContext_;


};

#pragma pack(pop)
} // namespace rir

#endif
