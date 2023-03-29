#include "DispatchTable.h"

#include "utils/BitcodeLinkUtility.h"
#include "utils/WorklistManager.h"
#include "utils/Hast.h"
#include "utils/deserializerData.h"

namespace rir {

Function* DispatchTable::get(size_t i) const {
    assert(i < capacity());
    // If there exists a L2 dispatch table at this index,
    // then check if there is a possible dispatch available
    SEXP funContainer = getEntry(i);

    if (L2Dispatch::check(funContainer)) {
        L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
        // print(std::cout);
        return l2vt->dispatch();
    }
    return Function::unpack(getEntry(i));
}

rir::Context DispatchTable::getContext(size_t i) const {
    assert(i < capacity());
    SEXP funContainer = getEntry(i);
    if (L2Dispatch::check(funContainer)) {
        L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
        return l2vt->getFallback()->context();
    }
    return Function::unpack(getEntry(i))->context();
}

void DispatchTable::print(std::ostream& out, const int & space) const {
    printSpace(space, out);
    out << "=== Printing Dispatch table (" << this << ") ===" << std::endl;
    // out << std::endl;
    // out << "Full Runtime speculative context" << std::endl;
    // Hast::printRawFeedback(this, out, space);
    // out << std::endl;
    for (size_t i = 1; i < size(); ++i) {
        SEXP funContainer = getEntry(i);

        if (L2Dispatch::check(funContainer)) {
            L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
            // auto fun = l2vt->dispatch();
            printSpace(space+2, out);
            out << "(" << i << "): " << l2vt->getFallback()->context() << std::endl;
            l2vt->print(out, space+4);
        } else {
            auto fun = Function::unpack(funContainer);
            printSpace(space+2, out);
            out << "(" << i << ")[" << (fun->disabled() ? "D" : "") << "]: " << fun->context() << std::endl;
        }
    }
}

void DispatchTable::insertL2(Function* fun) {
    assert(fun->signature().optimization !=
           FunctionSignature::OptimizationLevel::Baseline);

    int idx = negotiateSlot(fun->context());
    SEXP idxContainer = getEntry(idx);

    fun->vtab = this;

    if (idxContainer == R_NilValue) {
        Protect p;
        std::vector<SEXP> defaultArgs;
        size_t functionSize = sizeof(Function);
        SEXP store;
        p(store = Rf_allocVector(EXTERNALSXP, functionSize));
        void* payload = INTEGER(store);
        Function* dummy = new (payload) Function(functionSize, baseline()->body()->container(),
                                            defaultArgs, fun->signature(), fun->context());
        dummy->registerDeopt();
        dummy->vtab = this;
        L2Dispatch * l2vt = L2Dispatch::create(dummy, p);
        l2vt->insert(fun);
        fun->l2Dispatcher = l2vt;
        setEntry(idx, l2vt->container());
    } else {
        if (Function::check(idxContainer)) {
            Protect p;
            Function * old = Function::unpack(idxContainer);
            L2Dispatch * l2vt = L2Dispatch::create(old, p);
            l2vt->insert(fun);
            fun->l2Dispatcher = l2vt;
            setEntry(idx, l2vt->container());
        } else if (L2Dispatch::check(idxContainer)) {
            L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);
            l2vt->insert(fun);
            fun->l2Dispatcher = l2vt;
        } else {
            Rf_error("Dispatch table L2insertion error, corrupted slot!!");
        }
    }
}

void DispatchTable::insert(Function* fun) {
    assert(fun->signature().optimization !=
            FunctionSignature::OptimizationLevel::Baseline);
    int idx = negotiateSlot(fun->context());
    SEXP idxContainer = getEntry(idx);

    fun->vtab = this;

    if (idxContainer == R_NilValue) {
        setEntry(idx, fun->container());
    } else {
        if (Function::check(idxContainer)) {
            // Already existing container, do what is meant to be done
            if (idx != 0) {
                // Remember deopt counts across recompilation to avoid
                // deopt loops
                Function * old = Function::unpack(idxContainer);
                fun->addDeoptCount(old->deoptCount());
                setEntry(idx, fun->container());
                assert(get(idx) == fun);
            }
        } else if (L2Dispatch::check(idxContainer)) {
            L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);
            Function * old = l2vt->getFallback();
            fun->addDeoptCount(old->deoptCount());
            l2vt->setFallback(fun);
        } else {
            Rf_error("Dispatch table insertion error, corrupted slot!!");
        }
    }
}

int DispatchTable::negotiateSlot(const Context& assumptions) {
    assert(size() > 0);
    size_t i;
    for (i = size() - 1; i > 0; --i) {
        auto old = get(i);
        if (old->context() == assumptions) {
            // We already gave this context, dont delete it, just return the index
            return i;
        }
        if (!(assumptions < get(i)->context())) {
            break;
        }
    }
    i++;
    assert(!contains(assumptions));
    if (size() == capacity()) {
#ifdef DEBUG_DISPATCH
        std::cout << "Tried to insert into a full Dispatch table. Have: \n";
        for (size_t i = 0; i < size(); ++i) {
            auto e = getEntry(i);
            std::cout << "* " << Function::unpack(e)->context() << "\n";
        }
        std::cout << "\n";
        std::cout << "Tried to insert: " << assumptions << "\n";
        Rf_error("dispatch table overflow");
#endif
        // Evict one element and retry
        auto pos = 1 + (Random::singleton()() % (size() - 1));
        size_--;
        while (pos < size()) {
            setEntry(pos, getEntry(pos + 1));
            pos++;
        }
        return negotiateSlot(assumptions);
    }


    for (size_t j = size(); j > i; --j)
        setEntry(j, getEntry(j - 1));
    size_++;

    // Slot i is now available for insertion of context now
    setEntry(i, R_NilValue);
    return i;

#ifdef DEBUG_DISPATCH
    std::cout << "Added version to DT, new order is: \n";
    for (size_t i = 0; i < size(); ++i) {
        auto e = getEntry(i);
        std::cout << "* " << Function::unpack(e)->context() << "\n";
    }
    std::cout << "\n";
    for (size_t i = 0; i < size() - 1; ++i) {
        assert(get(i)->context() < get(i + 1)->context());
        assert(get(i)->context() != get(i + 1)->context());
        assert(!(get(i + 1)->context() < get(i)->context()));
    }
    assert(contains(fun->context()));
#endif
}

} // namespace rir
