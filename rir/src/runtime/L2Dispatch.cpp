#include "Function.h"


#include "L2Dispatch.h"

#include "runtime/DispatchTable.h"
namespace rir {


//
// V = 1 dispatch, Here we only dispatch to the latest unlocked binary and move towards more generic ones if we deopt
//
Function * L2Dispatch::V1Dispatch() {

	SEXP functionVector = getEntry(FVEC);

	// 0 - off
	// 1 - generic
	// 2 - specialized
	static int REV_DISPATCH = getenv("REV_DISPATCH") ? std::stoi(getenv("REV_DISPATCH")) : 0;

	switch(REV_DISPATCH) {
		case 1: {
			SEXP currFunHolder = VECTOR_ELT(functionVector, 0);
			auto res = Function::unpack(currFunHolder);
			// ensure res is not disabled
			if (!res->disabled()) {
				return Function::unpack(currFunHolder);
			} else {
				return Function::unpack(getGenesisFunctionContainer());
			}
			break;
		}
		case 2: {
			SEXP currFunHolder = VECTOR_ELT(functionVector, _last);
			auto res = Function::unpack(currFunHolder);
			// ensure res is not disabled
			if (!res->disabled()) {
				return Function::unpack(currFunHolder);
			} else {
				return Function::unpack(getGenesisFunctionContainer());
			}
			break;
		}
	}


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
	ObservedValues* observedTF = getBCSlots();
	SEXP functionVector = getEntry(FVEC);
	SEXP functionTFVector = getEntry(TVEC);

	// 0 - off
	// 1 - generic
	// 2 - specialized
	static int REV_DISPATCH = getenv("REV_DISPATCH") ? std::stoi(getenv("REV_DISPATCH")) : 0;

	switch(REV_DISPATCH) {
		case 1: {
			SEXP currFunHolder = VECTOR_ELT(functionVector, 0);
			auto res = Function::unpack(currFunHolder);
			// ensure res is not disabled
			if (!res->disabled()) {
				return Function::unpack(currFunHolder);
			} else {
				return Function::unpack(getGenesisFunctionContainer());
			}
			break;
		}
		case 2: {
			SEXP currFunHolder = VECTOR_ELT(functionVector, _last);
			auto res = Function::unpack(currFunHolder);
			// ensure res is not disabled
			if (!res->disabled()) {
				return Function::unpack(currFunHolder);
			} else {
				return Function::unpack(getGenesisFunctionContainer());
			}
			break;
		}
	}

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
			if (getFeedbackAsUint(observedTF[j]) != getFeedbackAsUint(currTF[j])) match = false;
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
