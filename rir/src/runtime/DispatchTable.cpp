#include "DispatchTable.h"

#include "utils/BitcodeLinkUtility.h"
namespace rir {
    void DispatchTable::tryLinking(SEXP currHastSym, const unsigned long & con, const int & nargs) {
        BitcodeLinkUtil::tryUnlockingOpt(hast, con, nargs);
    }

    void DispatchTable::insertL2V2(Function* fun, SEXP uEleContainer) {

        // doFeedbackRun = true;

        assert(fun->signature().optimization !=
               FunctionSignature::OptimizationLevel::Baseline);
        int idx = negotiateSlot(fun->context());

        SEXP idxContainer = getEntry(idx);

        if (idxContainer == R_NilValue) {
            Protect p;
            // Creation of a new L2V2 table takes place if slot is null
            SEXP slotsInfo = UnlockingElement::getTFSlotInfo(uEleContainer);
            std::vector<int> indices;
            std::vector<ObservedValues*> BCTFSlots;

            for (int i = 0; i < Rf_length(slotsInfo); i++) {
                indices.push_back(Rf_asInteger(VECTOR_ELT(slotsInfo, i)));
            }

            BitcodeLinkUtil::getTypeFeedbackPtrsAtIndices(indices, BCTFSlots, this);

            // Debug
            // std::cout << "Current Slot Vals: [ ";
            // for (auto & ele : BCTFSlots) {
            //     std::cout << *((uint32_t*) ele) << " ";
            // }
            // std::cout << "]" << std::endl;

            std::vector<ObservedValues> TVals;
            SEXP funTF = UnlockingElement::getFunTFInfo(uEleContainer);
            for (int i = 0; i < Rf_length(funTF); i++) {
                auto ele = generalUtil::getUint32t(funTF, i);
                TVals.push_back( *((ObservedValues *) &ele) ); // Casting uint32_t to Observed Value
            }

            // Debug
            // std::cout << "Current Fun: [ ";
            // for (auto & ele : TVals) {
            //     std::cout << *((uint32_t*) &ele) << " ";
            // }
            // std::cout << "]" << std::endl;

            std::vector<SEXP> defaultArgs;
            size_t functionSize = sizeof(Function);

            SEXP store;
            p(store = Rf_allocVector(EXTERNALSXP, functionSize));
            void* payload = INTEGER(store);
            Function* dummy = new (payload) Function(functionSize, baseline()->body()->container(),
                                                defaultArgs, fun->signature(), fun->context());

            dummy->registerDeopt();

            L2Dispatch * l2vt = L2Dispatch::create(dummy, BCTFSlots, p);
            l2vt->insert(fun, TVals);

            setEntry(idx, l2vt->container());
        } else {
            if (Function::check(idxContainer)) {
                // Protect p;
                // Function * old = Function::unpack(idxContainer);
                // L2Dispatch * l2vt = L2Dispatch::create(old, p);
                // setEntry(idx, l2vt->container());
                // l2vt->insert(fun);

                Protect p;
                // Creation of a new L2V2 table takes place if slot is null
                SEXP slotsInfo = UnlockingElement::getTFSlotInfo(uEleContainer);
                std::vector<int> indices;
                std::vector<ObservedValues*> BCTFSlots;

                for (int i = 0; i < Rf_length(slotsInfo); i++) {
                    indices.push_back(Rf_asInteger(VECTOR_ELT(slotsInfo, i)));
                }

                BitcodeLinkUtil::getTypeFeedbackPtrsAtIndices(indices, BCTFSlots, this);

                std::vector<ObservedValues> TVals;
                SEXP funTF = UnlockingElement::getFunTFInfo(uEleContainer);
                for (int i = 0; i < Rf_length(funTF); i++) {
                    auto ele = generalUtil::getUint32t(funTF, i);
                    TVals.push_back( *((ObservedValues *) &ele) ); // Casting uint32_t to Observed Value
                }

                Function * old = Function::unpack(idxContainer);

                L2Dispatch * l2vt = L2Dispatch::create(old, BCTFSlots, p);
                l2vt->insert(fun, TVals);
            } else if (L2Dispatch::check(idxContainer)) {
                L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);

                std::vector<ObservedValues> TVals;
                SEXP funTF = UnlockingElement::getFunTFInfo(uEleContainer);
                for (int i = 0; i < Rf_length(funTF); i++) {
                    auto ele = generalUtil::getUint32t(funTF, i);
                    TVals.push_back( *((ObservedValues *) &ele) ); // Casting uint32_t to Observed Value
                }

                l2vt->insert(fun, TVals);
            } else {
                Rf_error("Dispatch table L2insertion error, corrupted slot!!");
            }
        }

        if (hast) {
            tryLinking(hast, fun->context().toI(), fun->signature().numArguments);
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
                l2vt->insertGenesis(fun);
            } else {
                Rf_error("Dispatch table insertion error, corrupted slot!!");
            }
        }

        if (hast) {
            tryLinking(hast, fun->context().toI(), fun->signature().numArguments);
        }
    }

    void DispatchTable::insertL2V1(Function* fun) {
        // doFeedbackRun = true;
        assert(fun->signature().optimization !=
               FunctionSignature::OptimizationLevel::Baseline);
        int idx = negotiateSlot(fun->context());

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

            L2Dispatch * l2vt = L2Dispatch::create(dummy, p);
            l2vt->insert(fun);

            setEntry(idx, l2vt->container());
        } else {
            if (Function::check(idxContainer)) {
                Protect p;
                Function * old = Function::unpack(idxContainer);
                L2Dispatch * l2vt = L2Dispatch::create(old, p);
                setEntry(idx, l2vt->container());
                l2vt->insert(fun);
            } else if (L2Dispatch::check(idxContainer)) {
                L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);
                l2vt->insert(fun);
            } else {
                Rf_error("Dispatch table L2insertion error, corrupted slot!!");
            }
        }

        if (hast) {
            tryLinking(hast, fun->context().toI(), fun->signature().numArguments);
        }
    }

} // namespace rir
