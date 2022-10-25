#include "Function.h"


#include "L2Dispatch.h"

#include "runtime/DispatchTable.h"
namespace rir {


//
// V = 1 dispatch, Here we only dispatch to the latest unlocked binary and move towards more generic ones if we deopt
//
Function * L2Dispatch::V1Dispatch() {

	SEXP functionVector = getEntry(FVEC);

	for (int i = _last; i >= 0; i--) {
		SEXP currFunHolder = VECTOR_ELT(functionVector, i);
		Function * currFun = Function::unpack(currFunHolder);
		if (!currFun->disabled()) {
			#if DEBUG_HIT_MISS == 1
			std::cout << "V1 HIT" << std::endl;
			#endif
			return currFun;
		}
	}

	#if DEBUG_HIT_MISS == 1
	std::cout << "V1 MISS" << std::endl;
	#endif

	//
	// If all of these are disabled, then we dispatch to the genesis function.
	// In the beginning, genesis is just a dummy disabled function, but can later
	// be replaced by a JIT compiled version.
	//
	return Function::unpack(getGenesisFunctionContainer());
}


Function * L2Dispatch::V2Dispatch() {
	// ObservedValues* observedTF = getBCSlots();

	SEXP functionVector = getEntry(FVEC);
	SEXP functionTFVector = getEntry(TVEC);
	SEXP GFVector       = getEntry(GVEC);
	SEXP BCGVector = getEntry(BCGVEC);

	GenFeedbackHolder * GENSlots = (GenFeedbackHolder *) DATAPTR(BCGVector);


	// GenFeedbackHolder * stored = (GenFeedbackHolder *) DATAPTR(getEntry(BCGVEC));
	// for (unsigned int i = 0; i < _numGenSlots; i++) {
	// 	//
	// 	// Note: Pointer-pointers are so weird
	// 	//
	// 	std::cout << "[d] Stored at idx (" << i << "): " << stored[i].code << ", " << stored[i].pc << ", " << stored[i].tests << std::endl;
	// }

	// for (unsigned int i = 0; i < _numGenSlots; i++) {
	// 	//
	// 	// Note: Pointer-pointers are so weird
	// 	//
	// 	std::cout << "[GENSlots] Stored at idx (" << i << "): " << GENSlots[i].code << ", " << GENSlots[i].pc << ", " << GENSlots[i].tests << std::endl;
	// }




	// In this dispatch, only one type version is assumed so latest linked and available method is dispatched
	for (int i = _last; i >= 0; i--) {
		SEXP currFunHolder = VECTOR_ELT(functionVector, i);
		SEXP currTFHolder = VECTOR_ELT(functionTFVector, i);


		Function * currFun = Function::unpack(currFunHolder);

		ObservedValues* currTF = (ObservedValues *) DATAPTR(currTFHolder);


		bool match = true;

		// std::cout << "Checking Function [" << currFun << "]: " << i << "(" << currFun->disabled() << ")" << std::endl;

		for (unsigned int j = 0; j < _numSlots; j++) {
			// std::cout << "Slot[" << j << "]: " << getFeedbackAsUint(observedTF[j]) << ", " << getFeedbackAsUint(currTF[j]) << std::endl;
			if (getFeedbackAsUint(*BCTFSlots[j]) != getFeedbackAsUint(currTF[j])) match = false;
		}

		if (match) {

			SEXP currFunIntData = VECTOR_ELT(GFVector, i);
			int * tmp1 = (int *) DATAPTR(currFunIntData);
			std::vector<int> currentFunctionGenData;
			// std::cout << "CurrFunData: [ ";
			for (unsigned int m = 0; m < _numGenSlots; m++) {
				// std::cout  << tmp1[m] << " ";
				currentFunctionGenData.push_back(tmp1[m]);
			}
			// std::cout << "]" << std::endl;

			for (unsigned int j = 0; j < currentFunctionGenData.size(); j++) {

				auto currFunFeedbackId = currentFunctionGenData[j];
				if (currFunFeedbackId == 0) {
					// Nada, maybe check?
				} else if (currFunFeedbackId < 0) {
					if (GENSlots[j].tests) {
						// Set match is false if values are not same
						BC bc = BC::decode(GENSlots[j].tests, GENSlots[j].code);
						ObservedTest prof = bc.immediate.testFeedback;
						if (prof.seen == ObservedTest::OnlyTrue && currFunFeedbackId == -2) {
							// -2 == OnlyFalse
							match = false;
							// std::cout << "[-2 fail]" << std::endl;
						} else if (prof.seen == ObservedTest::OnlyFalse && currFunFeedbackId == -1) {
							// -1 == OnlyTrue
							match = false;
							// std::cout << "[-1 fail]" << std::endl;
						}
					} else {
						// std::cout << "LOOKUP INDEX: " << j << std::endl;
						// std::cout << "[NO PC error, test slot]: " << GENSlots[j].pc << ", IAM: " << currFun << std::endl;
						// Rf_error("NO PC ERROR");
					}

				} else {
					// Match function sources

					if (GENSlots[j].pc) {
						BC bc = BC::decode(GENSlots[j].pc, GENSlots[j].code);

						ObservedCallees prof = bc.immediate.callFeedback;

						if (prof.numTargets == 1) {
							SEXP currClos = prof.getTarget(GENSlots[j].code, 0);
							assert(TYPEOF(currClos) == CLOSXP);
							SEXP currBody = BODY(currClos);
							if (TYPEOF(currBody) == EXTERNALSXP && DispatchTable::check(currBody)) {
								int srcsrc = DispatchTable::unpack(currBody)->baseline()->body()->src;
								if (srcsrc != currFunFeedbackId) {
									match = false;
									// std::cout << "[L2 Final Check Fail]" << std::endl;
								}
								// std::cout << "[Call Match successful]" << std::endl;
							} else {
								match = false;
								// std::cout << "[L2 Body Check Fail]" << std::endl;
								// std::cout << "[TYPEOF]" << TYPEOF(currBody) << std::endl;
								// std::cout << "[DispatchTable]" << DispatchTable::check(currBody) << std::endl;
							}
						} else {
							// std::cout << "[L2 mono fail]" << std::endl;
						}

					} else {
						// std::cout << "[NO PC error]: " << GENSlots[j].pc << ", IAM: " << currFun << std::endl;
					}

				}
			}
		}

		if (match && !currFun->disabled()) {
			#if DEBUG_HIT_MISS == 1
			std::cout << "V2 HIT" << std::endl;
			#endif
			return currFun;
		}
	}
	#if DEBUG_HIT_MISS == 1
	std::cout << "V2 MISS" << std::endl;
	#endif
	//
	// If this fails we return to the genesisFunction, this might be a dummy function or a JIT compiled function
	//	The reason we dont fallback to V1 dispatch is that a Type Version may get unnecessarily disabled if we randomly
	// 	dispatch to it without checking type feedback, which will make is unusable in the future. To prevent this we
	// 	rely on the genesis function.
	//
	// return V1Dispatch();

	return Function::unpack(getGenesisFunctionContainer());
}

void L2Dispatch::insert(Function* f, const std::vector<ObservedValues> & TVals, const std::vector<int> & GFBVals) {
	// Expand the vector if capacity is full
	if (_last + 1 == capacity()) {
		expandStorage();
	}
	assert(_last + 1 < capacity());
	SEXP FunctionVector = getEntry(FVEC);
	SEXP TFVector       = getEntry(TVEC);
	SEXP GFVector       = getEntry(GVEC);

	_last++;

	SET_VECTOR_ELT(FunctionVector, _last, f->container());

	// Create a store of ObservedValues for the TFValues
	SEXP store;
	Protect protecc;
	// Note: instead of pointers, we store the values here instead
	protecc(store = Rf_allocVector(RAWSXP, _numSlots * sizeof(ObservedValues)));

	ObservedValues * tmp = (ObservedValues *) DATAPTR(store);

	for (unsigned int i = 0; i < _numSlots; i++) {
		tmp[i] = TVals[i];
	}

	SET_VECTOR_ELT(TFVector, _last, store);


	// Create a store of src indices
	// Note: instead of pointers, we store the values here instead
	protecc(store = Rf_allocVector(RAWSXP, GFBVals.size() * sizeof(int)));

	int * tmp1 = (int *) DATAPTR(store);

	for (unsigned int i = 0; i < GFBVals.size(); i++) {
		tmp1[i] = GFBVals[i];
	}

	SET_VECTOR_ELT(GFVector, _last, store);
}

L2Dispatch::L2Dispatch(Function* genesis, const std::vector<ObservedValues*> & L2Slots, const std::vector<GenFeedbackHolder> & GenL2Slots):
			RirRuntimeObject(sizeof(L2Dispatch),ENTRIES_SIZE),
	_versioning(2) {

		_numSlots = L2Slots.size();
		_numGenSlots = GenL2Slots.size();
		Protect protecc;
		//
		// 1. Populate BCVEC entries to point to ByteCode Locations
		//
		//

		BCTFSlots = L2Slots;

		SEXP BCGVector;
		// We make it a contigious big object, so we can directly index without VECTOR_ELT for each slot
		protecc(BCGVector = Rf_allocVector(RAWSXP, GenL2Slots.size() * sizeof(GenFeedbackHolder)));
		GenFeedbackHolder* tmp = (GenFeedbackHolder *) DATAPTR(BCGVector);

		// ObservedValues* workWith = *tmp;

		for (unsigned int i = 0; i < GenL2Slots.size(); i++) {
			GenFeedbackHolder ele = GenL2Slots[i];
			//
			// Note: Pointer-pointers are so weird
			//
			// std::cout << "Storing at idx (" << i << "): " << ele.code << ", " << ele.pc << ", " << ele.tests << std::endl;
			tmp[i] = ele;
		}

		// // Store BCVEC in the GC area
		setEntry(BCGVEC, BCGVector);

		// GenFeedbackHolder * stored = (GenFeedbackHolder *) DATAPTR(getEntry(BCGVEC));
		// for (unsigned int i = 0; i < GenL2Slots.size(); i++) {
		// 	//
		// 	// Note: Pointer-pointers are so weird
		// 	//
		// 	std::cout << "Stored at idx (" << i << "): " << stored->code << ", " << stored->pc << ", " << stored->tests << std::endl;
		// }



		//
		// 2. Populate Function Vector and its corresponding TFVector
		//

		SEXP FunctionVector, TFVector, GFVector;
		protecc(FunctionVector = Rf_allocVector(VECSXP, GROWTH_RATE));
		protecc(TFVector = Rf_allocVector(VECSXP, GROWTH_RATE));
		protecc(GFVector = Rf_allocVector(VECSXP, GROWTH_RATE));

		_last = -1;

		insertGenesis(genesis);

		// Store the vector in the GC area
		setEntry(FVEC, FunctionVector);
		setEntry(TVEC, TFVector);
		setEntry(GVEC, GFVector);


	}

} // namespace rir
