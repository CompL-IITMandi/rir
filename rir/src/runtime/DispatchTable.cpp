#include "DispatchTable.h"

#include "utils/BitcodeLinkUtility.h"
#define DEBUG_L2_ENTRIES 0

namespace rir {
    void DispatchTable::tryLinking(SEXP currHastSym, const unsigned long & con, const int & nargs) {
        BitcodeLinkUtil::tryUnlockingOpt(hast, con, nargs);
    }

    void DispatchTable::insertL2V2(Function* fun, SEXP uEleContainer) {
        fun->setVersioned(this->container());


        SEXP FBData = UnlockingElement::getGFunTFInfo(uEleContainer);
        #if DEBUG_L2_ENTRIES > 0
        std::cout << "insertL2V2 Start: " << fun << std::endl;
        #endif

        std::vector<int> funGFBData;

        for (int i = 0; i < Rf_length(FBData); i++) {
            auto ele = VECTOR_ELT(FBData, i);
            if (ele == R_NilValue) {
                funGFBData.push_back(0);
                // std::cerr << "NIL ";
            } else if (ele == R_dot_defined) {
                funGFBData.push_back(-1);
                // std::cerr << "T ";
            } else if (ele == R_dot_Method) {
                funGFBData.push_back(-2);
                // std::cerr << "F ";
            } else if (TYPEOF(ele) == VECSXP) {
                auto hast = VECTOR_ELT(ele, 0);
                auto index = Rf_asInteger(VECTOR_ELT(ele, 1));

                auto c = BitcodeLinkUtil::getCodeObjectAtOffset(hast, index);
                funGFBData.push_back(c->src);
                // std::cout << "[L2 unlock adding](" << CHAR(PRINTNAME(hast)) << "," << index << ") -> " << c->src;
            } else {
                funGFBData.push_back(0);
                // std::cout << "UN ";
            }
        }

        // std::cout << std::endl;

        #if DEBUG_L2_ENTRIES > 0
        std::cout << "[FUN] GNRL FEEDBACK: [";
        for (auto & ele : funGFBData) {
            std::cout << ele << " ";
        }
        std::cout << "]" << std::endl;
        #endif

        std::vector<ObservedValues> TVals;
        SEXP funTF = UnlockingElement::getFunTFInfo(uEleContainer);
        for (int i = 0; i < Rf_length(funTF); i++) {
            auto ele = generalUtil::getUint32t(funTF, i);
            TVals.push_back( *((ObservedValues *) &ele) ); // Casting uint32_t to Observed Value
        }

        #if DEBUG_L2_ENTRIES > 0
        std::cout << "[FUN] TYPE FEEDBACK: [";
        for (auto & ele : TVals) {
            std::cout << *((uint32_t*) &ele) << " ";
        }
        std::cout << "]" << std::endl;
        #endif

        // Populate general feedback
        SEXP ssInfo = UnlockingElement::getGTFSlotInfo(uEleContainer);
        std::vector<int> indices1;
        std::vector<GenFeedbackHolder> GENSlots;

        for (int i = 0; i < Rf_length(ssInfo); i++) {
            indices1.push_back(Rf_asInteger(VECTOR_ELT(ssInfo, i)));
        }

        BitcodeLinkUtil::getGeneralFeedbackPtrsAtIndices(indices1, GENSlots, this);

        #if DEBUG_L2_ENTRIES > 0
        std::cout << "[CODE] GEN SLOTS: [";
        for (auto & ele : GENSlots) {
            std::cout << "(" << ele.pc << ")";
            if (ele.tests) {
                std::cout << "[T] ";
            } else if (ele.pc) {
                std::cout << "[C](";
            } else {
                std::cout << "[E] ";
            }
        }
        std::cout << "]" << std::endl;
        #endif

        std::vector<int> indices;
        std::vector<ObservedValues*> BCTFSlots;

        SEXP slotsInfo = UnlockingElement::getTFSlotInfo(uEleContainer);
        for (int i = 0; i < Rf_length(slotsInfo); i++) {
            indices.push_back(Rf_asInteger(VECTOR_ELT(slotsInfo, i)));
        }

        BitcodeLinkUtil::getTypeFeedbackPtrsAtIndices(indices, BCTFSlots, this);

        #if DEBUG_L2_ENTRIES > 0
        std::cout << "[CODE] TF SLOTS: [";
        for (auto & ele : BCTFSlots) {
            std::cout << *((uint32_t *) &ele) << " ";
        }
        std::cout << "]" << std::endl;
        #endif


        // doFeedbackRun = true;


        assert(fun->signature().optimization !=
               FunctionSignature::OptimizationLevel::Baseline);

        int idx = negotiateSlot(fun->context());


        SEXP idxContainer = getEntry(idx);

        if (idxContainer == R_NilValue) {
            #if DEBUG_L2_ENTRIES > 0
            std::cout << "[L2] CASE 0: Creating a new L2 dispatch table" << std::endl;
            #endif

            Protect p;
            // Creation of a new L2V2 table takes place if slot is null



            std::vector<SEXP> defaultArgs;
            size_t functionSize = sizeof(Function);

            SEXP store;
            p(store = Rf_allocVector(EXTERNALSXP, functionSize));
            void* payload = INTEGER(store);
            Function* dummy = new (payload) Function(functionSize, baseline()->body()->container(),
                                                defaultArgs, fun->signature(), fun->context());

            dummy->registerDeopt();

            L2Dispatch * l2vt = L2Dispatch::create(dummy, BCTFSlots, GENSlots, p);
            l2vt->insert(fun, TVals, funGFBData);

            setEntry(idx, l2vt->container());
        } else {
            if (Function::check(idxContainer)) {
                #if DEBUG_L2_ENTRIES > 0
                std::cout << "[L2] CASE 1: Upgrading runtime to L2" << std::endl;
                #endif

                Protect p;
                Function * old = Function::unpack(idxContainer);

                L2Dispatch * l2vt = L2Dispatch::create(old, BCTFSlots, GENSlots, p);
                l2vt->insert(fun, TVals, funGFBData);
            } else if (L2Dispatch::check(idxContainer)) {
                #if DEBUG_L2_ENTRIES > 0
                std::cout << "[L2] CASE 2: Existing L2, adding to function list" << std::endl;
                #endif
                L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);

                l2vt->insert(fun, TVals, funGFBData);
            } else {
                Rf_error("Dispatch table L2insertion error, corrupted slot!!");
            }
        }

        #if DEBUG_L2_ENTRIES > 0
        std::cout << "insertL2V2 End, will try linking next!";
        #endif
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
                idxContainer = l2vt->getGenesisFunctionContainer();

                Function * old = Function::unpack(idxContainer);
                fun->addDeoptCount(old->deoptCount());

                // Check if genesis already exists, if yes copy deopt counts
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

        fun->setVersioned(this->container());

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
