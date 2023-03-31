#include "Function.h"
#include "L2Dispatch.h"
#include "runtime/DispatchTable.h"
#include <sstream>

#include <chrono>

using namespace std;

namespace rir {


static inline void printSpace(std::ostream& out, const int & space) {
	for (int i = 0; i < space; i++) {
		out << " ";
	}
}

std::string L2Dispatch::getInfo() {
	std::stringstream ss;

	if (_last == -1) {
		ss << "\"fallback\": " << "true" << ",";

	} else {
		ss << "\"fallback\": " << "false" << ",";

		unsigned total = 0, disabled = 0;
		for (int i = _last; i >= 0; i--) {
			auto currFun = getFunction(i);
			total++;
			ss << "\"" << currFun << "(" << (currFun->disabled() ? "Disabled" : "Enabled") << ")\": " << currFun->matchSpeculativeContext();
			ss << ",";
			if (currFun->disabled()) {
				disabled++;
			}
		}
		ss << "\"total\": " << total << ",";
		ss << "\"disabled\": " << disabled;
	}
	return ss.str();
}

void L2Dispatch::print(std::ostream& out, const int & space) {
	printSpace(out, space);
	out << "=== L2 dispatcher (" << this << ") ===" << std::endl;
	printSpace(out, space);
	out << "Fallback: " << (getFallback()->disabled() ? "[disabled]" : "[]") << std::endl;
	printSpace(out, space);
	out << "Function List(_last=" << _last << ")" << std::endl;
	for (int i = _last; i >= 0; i--) {
		auto currFun = getFunction(i);
		printSpace(out, space);
		out << "(" << i << ")" << (currFun->disabled() ? "[disabled]" : "[]") << "[function=" << currFun << "]";
		out << std::endl;

		currFun->printSpeculativeContext(out,space + 2);
	}

}
static bool l2Fastcase = getenv("L2_FASTCASE") ? getenv("L2_FASTCASE")[0] == '1' : true;

void L2Dispatch::insert(Function * f) {
	if (l2Fastcase)
		f->addFastcaseInvalidationConditions(&lastDispatch);
	int storageIdx = -1;
	for (int i = _last; i >= 0; i--) {
		Function * res = getFunction(i);
		if (res->disabled()) {
			if (lastDispatch == res) {
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

	SET_VECTOR_ELT(getEntry(FN_LIST), storageIdx, f->container());
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
	assert(false);
	return 0;
}

L2Dispatch::L2Dispatch(Function* fallback) :
	RirRuntimeObject(sizeof(L2Dispatch),ENTRIES_SIZE) {
	rir::Protect protecc;
	SEXP functionList;
	protecc(functionList = Rf_allocVector(VECSXP, GROWTH_RATE));
	setEntry(FN_LIST, functionList);

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
	auto fallback = getFallback();
	if (_last == -1) {
		assert(false && "Empty L2 dispatch");
		// if (EventLogger::logLevel >= 2) {
		// 	std::stringstream eventDataJSON;
		// 	eventDataJSON << "{"
		// 		<< "\"case\": " << "\"" << "empty" << "\"" << ","
		// 		<< "\"hast\": " << "\"" << (fallback->vtab->hast ? CHAR(PRINTNAME(fallback->vtab->hast)) : "NULL")  << "\"" << ","
		// 		<< "\"hastOffset\": " << "\"" << fallback->vtab->offsetIdx << "\"" << ","
		// 		<< "\"function\": " << "\"" << fallback << "\"" << ","
		// 		<< "\"context\": " << "\"" << fallback->context() << "\"" << ","
		// 		<< "\"vtab\": " << "\"" << fallback->vtab << "\"" << ","
		// 		<< "\"l2Info\": " << "{" << getInfo() << "}"
		// 		<< "}";

		// 	EventLogger::logUntimedEvent(
		// 		"l2Error",
		// 		eventDataJSON.str()
		// 	);
		// }
		return fallback;
	}

	if (lastDispatch && !lastDispatch->disabled()) {

		if (EventLogger::logLevel) {
				using namespace std::chrono;
				std::stringstream streamctx;
				streamctx << lastDispatch->context();

				std::stringstream streamname;
				streamname << lastDispatch;

				auto start = std::chrono::high_resolution_clock::now();

				EventLogger::logStats("l2Fast", streamname.str(),  0, start, streamctx.str(), nullptr, 0);
		}


		if (EventLogger::logLevel >= 2) {
			std::stringstream eventDataJSON;
			eventDataJSON << "{"
				<< "\"case\": " << "\"" << "fast" << "\"" << ","
				<< "\"hast\": " << "\"" << (lastDispatch->vtab->hast ? CHAR(PRINTNAME(lastDispatch->vtab->hast)) : "NULL")  << "\"" << ","
				<< "\"hastOffset\": " << "\"" << lastDispatch->vtab->offsetIdx << "\"" << ","
				<< "\"function\": " << "\"" << lastDispatch << "\"" << ","
				<< "\"context\": " << "\"" << lastDispatch->context() << "\"" << ","
				<< "\"vtab\": " << "\"" << lastDispatch->vtab << "\"" << ","
				<< "\"l2Info\": " << "{" << getInfo() << "}"
				<< "}";

			EventLogger::logUntimedEvent(
				"l2Fast",
				eventDataJSON.str()
			);
		}
		return lastDispatch;
	}

	for (int i = _last; i >= 0; i--) {
		auto currFun = getFunction(i);
		if (!currFun->disabled() && currFun->matchSpeculativeContext()) {
			lastDispatch = currFun;

			if (EventLogger::logLevel) {
				using namespace std::chrono;
				std::stringstream streamctx;
				streamctx << lastDispatch->context();

				std::stringstream streamname;
				streamname << lastDispatch;

				auto start = std::chrono::high_resolution_clock::now();

				EventLogger::logStats("l2Slow", streamname.str(),  0, start, streamctx.str(), nullptr, 0);
			}


			if (EventLogger::logLevel >= 2) {
				std::stringstream eventDataJSON;
				eventDataJSON << "{"
					<< "\"case\": " << "\"" << "slow" << "\"" << ","
					<< "\"hast\": " << "\"" << (lastDispatch->vtab->hast ? CHAR(PRINTNAME(lastDispatch->vtab->hast)) : "NULL")  << "\"" << ","
					<< "\"hastOffset\": " << "\"" << lastDispatch->vtab->offsetIdx << "\"" << ","
					<< "\"function\": " << "\"" << lastDispatch << "\"" << ","
					<< "\"context\": " << "\"" << lastDispatch->context() << "\"" << ","
					<< "\"vtab\": " << "\"" << lastDispatch->vtab << "\"" << ","
					<< "\"l2Info\": " << "{" << getInfo() << "}"
					<< "}";

				EventLogger::logUntimedEvent(
					"l2Slow",
					eventDataJSON.str()
				);
			}
			return currFun;
		}
	}

	if (EventLogger::logLevel) {
			using namespace std::chrono;
			std::stringstream streamctx;
			streamctx << fallback->context();

			std::stringstream streamname;
			streamname << fallback;

			auto start = std::chrono::high_resolution_clock::now();

			EventLogger::logStats("l2Miss", streamname.str(),  0, start, streamctx.str(), nullptr, 0);
	}


	if (EventLogger::logLevel >= 2) {
		std::stringstream eventDataJSON;
		eventDataJSON << "{"
			<< "\"case\": " << "\"" << "miss" << "\"" << ","
			<< "\"hast\": " << "\"" << (fallback->vtab->hast ? CHAR(PRINTNAME(fallback->vtab->hast)) : "NULL")  << "\"" << ","
			<< "\"hastOffset\": " << "\"" << fallback->vtab->offsetIdx << "\"" << ","
			<< "\"function\": " << "\"" << fallback << "\"" << ","
			<< "\"context\": " << "\"" << fallback->context() << "\"" << ","
			<< "\"vtab\": " << "\"" << fallback->vtab << "\"" << ","
			<< "\"l2Info\": " << "{" << getInfo() << "}"
			<< "}";

		EventLogger::logUntimedEvent(
			"l2Miss",
			eventDataJSON.str()
		);
	}

	if (l2Fastcase) {
		lastDispatch = fallback;
	}

	return fallback;
}

} // namespace rir
