#include "DispatchTable.h"

#define DEBUG_L2_ENTRIES 0

#include "utils/BitcodeLinkUtility.h"
namespace rir {
    void DispatchTable::tryLinking(SEXP currHastSym, const unsigned long & con, const int & nargs) {
        BitcodeLinkUtil::tryUnlockingOpt(hast, con, nargs);
    }

    void DispatchTable::insertL2V2(Function* fun, SEXP uEleContainer) {

        // 0 - off
		// 1 - generic
		// 2 - specialized
		static int REV_DISPATCH = getenv("REV_DISPATCH") ? std::stoi(getenv("REV_DISPATCH")) : 0;


        SEXP FBData = UnlockingElement::getGFunTFInfo(uEleContainer);
        #if DEBUG_L2_ENTRIES > 0
        std::cout << "insertL2V2 Start";
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
                // std::cout << "[L2 unlock adding](" << CHAR(PRINTNAME(hast)) << "," << index << ") -> ";
                auto c = BitcodeLinkUtil::getCodeObjectAtOffset(hast, index);
                // std::cout << "[Found src] -> " << c->src;
                funGFBData.push_back(c->src);
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

        fun->setVersioned(this->container());

        // doFeedbackRun = true;

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



        assert(fun->signature().optimization !=
               FunctionSignature::OptimizationLevel::Baseline);
        int idx = negotiateSlot(fun->context());

        SEXP idxContainer = getEntry(idx);

        if (idxContainer == R_NilValue) {
            #if DEBUG_L2_ENTRIES > 0
            std::cout << "Case 0: Empty Slot!" << std::endl;
            #endif
            Protect p;
            // Creation of a new L2V2 table takes place if slot is null
            SEXP slotsInfo = UnlockingElement::getTFSlotInfo(uEleContainer);
            std::vector<int> indices;
            std::vector<ObservedValues*> BCTFSlots;
            for (int i = 0; i < Rf_length(slotsInfo); i++) {
                indices.push_back(Rf_asInteger(VECTOR_ELT(slotsInfo, i)));
            }
            BitcodeLinkUtil::getTypeFeedbackPtrsAtIndices(indices, BCTFSlots, this);


            std::vector<SEXP> defaultArgs;
            size_t functionSize = sizeof(Function);

            SEXP store;
            p(store = Rf_allocVector(EXTERNALSXP, functionSize));
            void* payload = INTEGER(store);
            Function* dummy = new (payload) Function(functionSize, baseline()->body()->container(),
                                                defaultArgs, fun->signature(), fun->context());

            dummy->registerDeopt();

            #if DEBUG_L2_ENTRIES > 0
            std::cout << "(*) Creating L2" << std::endl;
            #endif


            L2Dispatch * l2vt = L2Dispatch::create(dummy, BCTFSlots, GENSlots, p);

            // General feedback data in function

            #if DEBUG_L2_ENTRIES > 0
            std::cout << "(*) Inserting function" << std::endl;
            #endif


            l2vt->insert(fun, TVals, funGFBData);

            setEntry(idx, l2vt->container());
        } else {
            if (Function::check(idxContainer)) {
                #if DEBUG_L2_ENTRIES > 0
                std::cout << "Case 1: Existing Function!" << std::endl;
                #endif
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


                Function * old = Function::unpack(idxContainer);

                #if DEBUG_L2_ENTRIES > 0
                std::cout << "(*) Creating L2" << std::endl;
                #endif

                L2Dispatch * l2vt = L2Dispatch::create(old, BCTFSlots, GENSlots, p);

                #if DEBUG_L2_ENTRIES > 0
                std::cout << "(*) Inserting function" << std::endl;
                #endif
                l2vt->insert(fun, TVals, funGFBData);
            } else if (L2Dispatch::check(idxContainer)) {
                if (REV_DISPATCH == 0) {
                    #if DEBUG_L2_ENTRIES > 0
                    std::cout << "Case 2: Existing L2!" << std::endl;
                    #endif

                    #if DEBUG_L2_ENTRIES > 0
                    std::cout << "(*) Inserting function" << std::endl;
                    #endif
                    L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);
                    l2vt->insert(fun, TVals, funGFBData);
                }

                // Special case when REV_DISPATCH == 2, then insert only if size is one
                if (REV_DISPATCH == 2) {
                    L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);
                    if (l2vt->entries() == 1) {
                        l2vt->insert(fun, TVals, funGFBData);
                    }
                }
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
        // 0 - off
		// 1 - generic
		// 2 - specialized
        static int REV_DISPATCH = getenv("REV_DISPATCH") ? std::stoi(getenv("REV_DISPATCH")) : 0;

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
                if (REV_DISPATCH == 0) {
                    L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);
                    l2vt->insert(fun);
                }

                // Special case when REV_DISPATCH == 2, then insert only if size is one
                if (REV_DISPATCH == 2) {
                    L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);
                    if (l2vt->entries() == 1) {
                        l2vt->insert(fun);
                    }
                }

            } else {
                Rf_error("Dispatch table L2insertion error, corrupted slot!!");
            }
        }

        if (hast) {
            tryLinking(hast, fun->context().toI(), fun->signature().numArguments);
        }
    }

} // namespace rir
