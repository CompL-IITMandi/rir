#include "Function.h"
#include "L2Dispatch.h"
#include "runtime/DispatchTable.h"

namespace rir {

static inline void printSpace(std::ostream& out, const int & space) {
	for (int i = 0; i < space; i++) {
		out << " ";
	}
}

void L2Dispatch::print(std::ostream& out, const int & space) {
	printSpace(out, space);
	out << "=== L2 dispatcher (" << this << ") ===" << std::endl;
	printSpace(out, space);
	out << "Fallback: " << (getFallback()->disabled() ? "[disabled]" : "[]") << std::endl;
	printSpace(out, space);
	out << "Runtime Speculative Context: ";
	for (auto & ele : runtimeFeedbackData) {
		ele.print(out);
		out << " ";
	}
	out << std::endl;
	printSpace(out, space);
	out << "Function List(_last=" << _last << ")" << std::endl;
	for (int i = _last; i >= 0; i--) {
		auto currFun = getFunction(i);
		printSpace(out, space);
		out << "(" << i << ")" << (currFun.fun->disabled() ? "[disabled]" : "[]");
		out << "Speculative Context: ";
		for (size_t j = 0; j < runtimeFeedbackData.size(); j++) {
			currFun.feedback[j].print(out);
			out << " ";
		}
		out << std::endl;
	}

}

void L2Dispatch::insert(Function * f, const std::vector<L2Feedback> & feedbackVals) {
	// std::cout << "=== Inserting new function ===" << std::endl;
	// std::cout << "Given Speculative Context: ";
	// for (auto & ele : feedbackVals) {
	// 	ele.print(std::cout);
	// 	std::cout << " ";
	// }
	// std::cout << std::endl;
	// Replace disabled functions
	int storageIdx = -1;
	for (int i = _last; i >= 0; i--) {
		L2Function res = getFunction(i);
		if (res.fun->disabled()) {
			if (lastDispatch == res.fun) {
				lastDispatch = nullptr;
			}
			storageIdx = i;
			break;
		}
	}
	if (storageIdx == -1) {
		if (_last + 1 == functionListContainerSize()) {
			expandStorage();
		}
		assert(_last + 1 < functionListContainerSize());
		_last++;
		storageIdx = _last;
	}
	SEXP mainContainer, feedbackContainer;
	Protect protecc;
	protecc(mainContainer = Rf_allocVector(VECSXP, 2));
	protecc(feedbackContainer = Rf_allocVector(RAWSXP, feedbackVals.size() * sizeof(L2Feedback)));

	L2Feedback* currTF = (L2Feedback *) DATAPTR(feedbackContainer);

	for (size_t i = 0; i < feedbackVals.size(); i++) {
		currTF[i] = feedbackVals[i];
	}
	// std::cout << "Saved Speculative Context: ";
	// for (size_t i = 0; i < feedbackVals.size(); i++) {
	// 	currTF[i].print(std::cout);
	// 	std::cout << " ";
	// }
	// std::cout << std::endl;

	{ // [Function, Speculative Context]
		SET_VECTOR_ELT(mainContainer, 0, f->container());
		SET_VECTOR_ELT(mainContainer, 1, feedbackContainer);
	}
	SET_VECTOR_ELT(getEntry(FN_LIST), storageIdx, mainContainer);

	// print(std::cout);
}

void L2Dispatch::expandStorage() {
	rir::Protect p;
	SEXP oldVec = getEntry(FN_LIST);
	int oldSize = Rf_length(oldVec);
	SEXP newVec;
	p(newVec = Rf_allocVector(VECSXP, oldSize + GROWTH_RATE));
	memcpy(DATAPTR(newVec), DATAPTR(oldVec), oldSize * sizeof(SEXP));
	setEntry(FN_LIST, newVec);
}

uint32_t L2Feedback::getTypeFeedbackVal() const {
	if (tag == 0) return fVal.typeFeedbackVal;
	else if (tag == 1) return *reinterpret_cast<uint32_t*>(fVal.typeFeedbackPtr);
	// std::cout << "getTypeFeedbackVal(tag=" << tag << ")" << std::endl;
	assert(false);
	return 0;
}

ObservedTest L2Feedback::getTestFeedbackVal() const {
	if (tag == 2) return fVal.testVal;
	else if(tag == 3) return *reinterpret_cast<ObservedTest*>(fPtr.pc);
	// std::cout << "getTestFeedbackVal(tag=" << tag << ")" << std::endl;
	assert(false);
	return ObservedTest();
}

int L2Feedback::getSrcIdxVal() const {
	if (tag == 4) return fVal.srcIdx;
	else if(tag == 5) {
		// std::cout << "getSrcIdxVal(tag=5): " << fPtr.pc << ", " << fPtr.code << std::endl;
		ObservedCallees * prof = (ObservedCallees *) fPtr.pc;
		// std::cout << "prof: " << prof->invalid << std::endl;
		if (prof->invalid) {
			return 0;
		}
		if (prof->numTargets > 0) {
			auto lastTargetIndex = prof->targets[prof->numTargets-1];
			SEXP currClos = fPtr.code->getExtraPoolEntry(lastTargetIndex);
			if (DispatchTable::check(BODY(currClos))) {
				return DispatchTable::unpack(BODY(currClos))->baseline()->body()->src;
			}
		}
		return 0;
	}
	// std::cout << "getSrcIdxVal(tag=" << tag << ")" << std::endl;
	//assert(false);
	return 0;
}

L2Dispatch::L2Dispatch(Function* fallback, const std::vector<L2Feedback> & feedbackVals) :
	RirRuntimeObject(sizeof(L2Dispatch),ENTRIES_SIZE) {
	rir::Protect protecc;
	SEXP functionList;
	protecc(functionList = Rf_allocVector(VECSXP, GROWTH_RATE));
	setEntry(FN_LIST, functionList);

	runtimeFeedbackData = feedbackVals;
	setFallback(fallback);
	// print(std::cout);
}

void L2Feedback::print(std::ostream& out, const int & space) const {
	for (int i = 0;i < space; i++) {
		out << " ";
	}

	if (tag < 2) {
		auto res = getTypeFeedbackVal();
		out << "<";
		reinterpret_cast<ObservedValues*>(&res)->print(out);
		out << ">";
		return;
	}


	if (tag < 4) {
		out << "<Branch[";
		switch (getTestFeedbackVal().seen) {
			case 0: out << "None"; break;
			case 1: out << "OnlyTrue"; break;
			case 2: out << "OnlyFalse"; break;
			case 3: out << "Both"; break;
		}
		out << "]>";
		return;
	}

	if (tag < 6) {
		out << "<CalleeAt[";
		out << getSrcIdxVal() << "]>";
		return;
	}
	out << "?>";
}

bool L2Feedback::operator==(const L2Feedback& other) {
	if (this->tag < 2) {
		return this->getTypeFeedbackVal() == other.getTypeFeedbackVal();
	}

	if (this->tag < 4) {
		if (other.tag == 6) return false;
		return this->getTestFeedbackVal().seen == other.getTestFeedbackVal().seen;
	}

	if (this->tag < 6) {
		if (other.tag == 6) return false;
		return this->getSrcIdxVal() == other.getSrcIdxVal();
	}

	return other.tag == 6;
}

Function * L2Dispatch::dispatch() {
	#if ASSERT_CONDITIONS == 1
	// If nothing exists return genesis
	if (_last == -1) {
		return getFallback();
	}
	#endif
	// std::cout << "L2 Dispatch called" << std::endl;
	// print(std::cout);


	if (lastDispatch && !lastDispatch->disabled()) {
		// std::cout << "=== L2Dispatch fastcase ===" << std::endl;
		return lastDispatch;
	}

	for (int i = _last; i >= 0; i--) {
		bool match = true;
		auto currFun = getFunction(i);
		if (currFun.fun->disabled()) {
			continue;
		}
		for (size_t j = 0; j < runtimeFeedbackData.size(); j++) {
			#if ASSERT_CONDITIONS == 1
			assert(runtimeFeedbackData.size() == currFun.feedback->size());
			#endif
			if (!(runtimeFeedbackData[j] == currFun.feedback[j])) {
				match = false;
				break;
			}
		}
		if (match) {
			// std::cout << "=== L2Dispatch success ===" << std::endl;
			lastDispatch = currFun.fun;
			return currFun.fun;
		}
	}
	// std::cout << "=== L2Dispatch fail ===" << std::endl;
	return getFallback();
}

} // namespace rir
