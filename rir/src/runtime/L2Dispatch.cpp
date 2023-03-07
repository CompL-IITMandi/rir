#include "Function.h"
#include "L2Dispatch.h"
#include "runtime/DispatchTable.h"

namespace rir {

uint32_t L2Feedback::getTypeFeedbackVal() const {
	if (tag == 0) return fVal.typeFeedbackVal;
	else if (tag == 1) return *reinterpret_cast<uint32_t*>(fVal.typeFeedbackPtr);
	assert(false);
	return 0;
}

ObservedTest L2Feedback::getTestFeedbackVal() const {
	if (tag == 2) return fVal.testVal;
	else if(tag == 3) return *reinterpret_cast<ObservedTest*>(fPtr.pc);
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
	assert(false);
	return 0;
}

void L2Feedback::print(std::ostream& out, const int & space) const {
	for (int i = 0;i < space; i++) {
		out << " ";
	}
	out << "<" << tag << ",";
	if (tag < 2) {
		out << getTypeFeedbackVal() << ">";
		return;
	}

	if (tag < 4) {
		out << getTestFeedbackVal().seen << ">";
		return;
	}

	if (tag < 6) {
		out << getSrcIdxVal() << ">";
		return;
	}
}

bool L2Feedback::operator==(const L2Feedback& other) {
	// std::cout << "Comparing: ";
	// this->print(std::cout);
	// std::cout << " ";
	// other.print(std::cout);
	// std::cout << std::endl;
	if (this->tag < 2) {
		return this->getTypeFeedbackVal() == other.getTypeFeedbackVal();
	}

	if (this->tag < 4) {
		return this->getTestFeedbackVal().seen == other.getTestFeedbackVal().seen;
	}

	if (this->tag < 6) {
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

	// std::cout << "L2 start" << std::endl;

	if (lastDispatch && !lastDispatch->disabled()) {
		// std::cout << "L2 fastcase" << std::endl;
		return lastDispatch;
	}

	for (int i = _last; i >= 0; i--) {
		// std::cout << "L2 dispatch: " << i << std::endl;
		bool match = true;
		auto currFun = getFunction(i);
		if (currFun.fun->disabled()) {
			// std::cout << "[disabled]" << std::endl;
			continue;
		}
		for (size_t j = 0; j < runtimeFeedbackData.size(); j++) {
			#if ASSERT_CONDITIONS == 1
			assert(runtimeFeedbackData.size() == currFun.feedback->size());
			#endif
			if (!(runtimeFeedbackData[j] == currFun.feedback[j])) {
				// std::cout << "[L2 Fail]" << std::endl;
				match = false;
				break;
			}
		}
		if (match) {
			// std::cout << "L2 match" << std::endl;
			lastDispatch = currFun.fun;
			return currFun.fun;
		}
	}
	// std::cout << "L2 miss (new context)" << std::endl;
	return getFallback();
}

} // namespace rir
