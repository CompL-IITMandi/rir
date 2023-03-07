#include "DispatchTable.h"

#include "utils/BitcodeLinkUtility.h"
#include "utils/WorklistManager.h"
#include "utils/Hast.h"
#include "utils/deserializerData.h"

namespace rir {

static std::vector<L2Feedback> getFunctionContext(SEXP container) {
    std::vector<L2Feedback> res;
    SEXP funTF = UnlockingElement::getFunTFInfo(container);
    // Populate type feedback
    for (int i = 0; i < Rf_length(funTF); i++) {
        auto ele = generalUtil::getUint32t(funTF, i);
        res.push_back(L2Feedback::create(ele, true));
    }
    // Populate other feedback
    SEXP FBData = UnlockingElement::getGFunTFInfo(container);
    for (int i = 0; i < Rf_length(FBData); i++) {
        auto ele = VECTOR_ELT(FBData, i);
        if (ele == R_NilValue) {
            res.push_back(L2Feedback::create());
        } else if (ele == R_dot_defined) {
            ObservedTest r;
            r.seen = ObservedTest::OnlyTrue;
            res.push_back(L2Feedback::create(r));
        } else if (ele == R_dot_Method) {
            ObservedTest r;
            r.seen = ObservedTest::OnlyFalse;
            res.push_back(L2Feedback::create(r));
        } else if (TYPEOF(ele) == VECSXP) {
            auto hast = VECTOR_ELT(ele, 0);
            auto index = Rf_asInteger(VECTOR_ELT(ele, 1));
            auto c = Hast::getCodeObjectAtOffset(hast, index);
            res.push_back(L2Feedback::create(c->src, false));
        } else {
            res.push_back(L2Feedback::create());
        }
    }
    // std::cout << "getFunctionContext: { ";

    // for (auto & ele : res) {
    //     ele.print(std::cout);
    //     std::cout << " ";
    // }

    // std::cout << "}" << std::endl;
    return res;
}

static std::vector<L2Feedback> getRuntimeContext(SEXP container, DispatchTable * vtab) {
    std::vector<L2Feedback> res;

    // Populate type feedback
    std::vector<int> indices;
    SEXP slotsInfo = UnlockingElement::getTFSlotInfo(container);
    for (int i = 0; i < Rf_length(slotsInfo); i++) {
        indices.push_back(Rf_asInteger(VECTOR_ELT(slotsInfo, i)));
    }
    std::vector<ObservedValues*> BCTFSlots;
    Hast::getTypeFeedbackPtrsAtIndices(indices, BCTFSlots, vtab);
    for (auto & ele : BCTFSlots) {
        res.push_back(L2Feedback::create(ele));
    }

    // Populate other feedback
    SEXP ssInfo = UnlockingElement::getGTFSlotInfo(container);
    std::vector<int> indices1;
    std::vector<GenFeedbackHolder> GENSlots;

    for (int i = 0; i < Rf_length(ssInfo); i++) {
        indices1.push_back(Rf_asInteger(VECTOR_ELT(ssInfo, i)));
    }

    Hast::getGeneralFeedbackPtrsAtIndices(indices1, GENSlots, vtab);

    for (auto & ele : GENSlots) {
        if (ele.tag == 0) { // record_call_
            res.push_back(L2Feedback::create(ele.code, ele.pc));
        } else { // record_test_
            res.push_back(L2Feedback::create(ele.pc));
        }
    }

    // std::cout << "getRuntimeContext: { ";

    // for (auto & ele : res) {
    //     ele.print(std::cout);
    //     std::cout << " ";
    // }

    // std::cout << "}" << std::endl;

    return res;
}

void DispatchTable::insertL2V2(Function* fun, SEXP uEleContainer) {
    assert(fun->signature().optimization !=
           FunctionSignature::OptimizationLevel::Baseline);

    int idx = negotiateSlot(fun->context());
    SEXP idxContainer = getEntry(idx);

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
        L2Dispatch * l2vt = L2Dispatch::create(dummy, getRuntimeContext(uEleContainer, this), p);
        l2vt->insert(fun, getFunctionContext(uEleContainer));

        setEntry(idx, l2vt->container());
    } else {
        if (Function::check(idxContainer)) {
            Protect p;
            Function * old = Function::unpack(idxContainer);
            L2Dispatch * l2vt = L2Dispatch::create(old, getRuntimeContext(uEleContainer, this), p);
            l2vt->insert(fun, getFunctionContext(uEleContainer));
        } else if (L2Dispatch::check(idxContainer)) {
            L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);
            l2vt->insert(fun, getFunctionContext(uEleContainer));
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

void DispatchTable::insertL2V1(Function* fun) {
    assert(fun->signature().optimization !=
            FunctionSignature::OptimizationLevel::Baseline);
    int idx = negotiateSlot(fun->context());
    std::vector<L2Feedback> feedbackVals;
    SEXP idxContainer = getEntry(idx);
    if (idxContainer == R_NilValue) {
        Protect p;
        // Create a dummy genesis function that cannot
        // be called i.e. is disabled() since creation
        std::vector<SEXP> defaultArgs;
        size_t functionSize = sizeof(Function);

        SEXP store;
        p(store = Rf_allocVector(EXTERNALSXP, functionSize));
        void* payload = INTEGER(store);
        Function* dummy = new (payload) Function(functionSize, baseline()->body()->container(),
                                            defaultArgs, fun->signature(), fun->context());
        dummy->registerDeopt();
        L2Dispatch * l2vt = L2Dispatch::create(dummy, feedbackVals, p);
        l2vt->insert(fun, feedbackVals);
        setEntry(idx, l2vt->container());
    } else {
        if (Function::check(idxContainer)) {
            Protect p;
            Function * old = Function::unpack(idxContainer);
            L2Dispatch * l2vt = L2Dispatch::create(old, feedbackVals, p);
            setEntry(idx, l2vt->container());
            l2vt->insert(fun, feedbackVals);
        } else if (L2Dispatch::check(idxContainer)) {
            L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);
            l2vt->insert(fun, feedbackVals);
        } else {
            Rf_error("Dispatch table L2insertion error, corrupted slot!!");
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
