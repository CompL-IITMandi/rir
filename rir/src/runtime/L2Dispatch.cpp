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
			std::vector<int> currentFunctionGenData;
			for (int m = 0; m < Rf_length(currFunIntData); m++) {
				currentFunctionGenData.push_back(INTEGER(currFunIntData)[m]);
			}

			for (unsigned int j = 0; j < currentFunctionGenData.size(); j++) {

				auto currFunFeedbackId = currentFunctionGenData[j];
				if (currFunFeedbackId == 0) {
					// Nada, maybe check?
				} else if (currFunFeedbackId < 0) {
					// Set match is false if values are not same
					BC bc = BC::decode(GENSlots[j].pc, GENSlots[j].code);
					ObservedTest prof = bc.immediate.testFeedback;
					if (prof.seen == ObservedTest::OnlyTrue && currFunFeedbackId == -2) {
						// -2 == OnlyFalse
						match = false;
						std::cout << "[-2 fail]" << std::endl;
					} else if (prof.seen == ObservedTest::OnlyFalse && currFunFeedbackId == -1) {
						// -1 == OnlyTrue
						match = false;
						std::cout << "[-1 fail]" << std::endl;
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
								std::cout << "[Call Match successful]" << std::endl;
							} else {
								match = false;
								std::cout << "[L2 Body Check Fail]" << std::endl;
								std::cout << "[TYPEOF]" << TYPEOF(currBody) << std::endl;
								std::cout << "[DispatchTable]" << DispatchTable::check(currBody) << std::endl;
							}
						} else {
							std::cout << "[L2 mono fail]" << std::endl;
						}

					} else {
						std::cout << "[NO PC error]: " << GENSlots[j].pc << std::endl;
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
} // namespace rir
