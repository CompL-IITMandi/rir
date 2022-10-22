#include "Function.h"


#include "L2Dispatch.h"

#include "runtime/DispatchTable.h"
namespace rir {


Function * L2Dispatch::V2Dispatch() {
		ObservedValues* observedTF = getBCSlots();

		GenFeedbackHolder* observedGeneralFB = getGFBSlots();

		SEXP functionVector = getEntry(FVEC);
		SEXP functionTFVector = getEntry(TVEC);
		SEXP functionGFBVector = getEntry(GVEC);

		// 0 - off
		// 1 - generic
		// 2 - specialized
		static int REV_DISPATCH = getenv("REV_DISPATCH") ? std::stoi(getenv("REV_DISPATCH")) : 0;

		switch(REV_DISPATCH) {
			case 1: {
				SEXP currFunHolder = VECTOR_ELT(functionVector, 0);
				return Function::unpack(currFunHolder);
				break;
			}
			case 2: {
				SEXP currFunHolder = VECTOR_ELT(functionVector, _last);
				return Function::unpack(currFunHolder);
				break;
			}
		}


		// In this dispatch, only one type version is assumed so latest linked and available method is dispatched
		for (int i = _last; i >= 0; i--) {
			SEXP currFunHolder = VECTOR_ELT(functionVector, i);
			SEXP currTFHolder = VECTOR_ELT(functionTFVector, i);

			SEXP currGFBHolder = VECTOR_ELT(functionGFBVector, i);

			Function * currFun = Function::unpack(currFunHolder);

			ObservedValues* currTF = (ObservedValues *) DATAPTR(currTFHolder);

			int* currGFB = (int *) DATAPTR(currGFBHolder);

			bool match = true;

			// std::cout << "Checking Function [" << currFun << "]: " << i << "(" << currFun->disabled() << ")" << std::endl;

			for (unsigned int j = 0; j < _numSlots; j++) {
				// std::cout << "Slot[" << j << "]: " << getFeedbackAsUint(observedTF[j]) << ", " << getFeedbackAsUint(currTF[j]) << std::endl;
				if (getFeedbackAsUint(observedTF[j]) != getFeedbackAsUint(currTF[j])) match = false;
			}

			//  TODO MATCH GENERAL FEEDBACK
			for (unsigned int j = 0; j < _numGenSlots; j++) {
				if (currGFB[j] == 0) {
					// Nil Value, assume match is true
				} else if (currGFB[j] > 0) {
					// Current call target must be monomorphic and must be equal to currGFB[j]
					// TODO: Call site check



					GenFeedbackHolder feedback = observedGeneralFB[j];
					if (feedback.pc) {
						BC bc = BC::decode(observedGeneralFB[j].pc, observedGeneralFB[j].code);

						ObservedCallees prof = bc.immediate.callFeedback;

						if (prof.numTargets == 1) {
							SEXP currClos = prof.getTarget(observedGeneralFB[j].code, 0);
							assert(TYPEOF(currClos) == CLOSXP);
							SEXP currBody = BODY(currClos);
							if (TYPEOF(currBody) == EXTERNALSXP && DispatchTable::check(currBody)) {
								int srcsrc = DispatchTable::unpack(currBody)->baseline()->body()->src;
								if (srcsrc != currGFB[j]) {
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
						// std::cout << "[NO PC error]" << std::endl;
					}
				} else {
					// Test site check
					// currGFB[j]
					void * ccc = &observedGeneralFB[j];
					ObservedTest * prof = (ObservedTest *) ccc;
					if (prof->seen == ObservedTest::OnlyTrue && currGFB[j] == -2) {
						// -2 == OnlyFalse
						match = false;
						// std::cout << "[-2 err]" << std::endl;
					} else if (prof->seen == ObservedTest::OnlyFalse && currGFB[j] == -1) {
						// -1 == OnlyTrue
						match = false;
						// std::cout << "[-1 err]" << std::endl;
					}
				}
				// std::cout << "Slot[" << j << "]: " << getFeedbackAsUint(observedTF[j]) << ", " << getFeedbackAsUint(currTF[j]) << std::endl;
			}

			if (match && !currFun->disabled()) {
				// std::cout << "[L2 HIT]" << std::endl;
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
