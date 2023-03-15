#pragma once
#include "Function.h"
#include "R/Protect.h"
namespace rir {

struct GenFeedbackHolder {
	unsigned tag;
    rir::Code * code;
    Opcode * pc;
};
struct FeedbackValue {
	uint32_t typeFeedbackVal;
	ObservedValues* typeFeedbackPtr;
	ObservedTest testVal;
	unsigned srcIdx;
};

struct L2Feedback {
	// TAG:
	// 0 -> Type Feedback Val
	// 1 -> Type Feedback Ptr
	// 2 -> Test Val
	// 3 -> Test Ptr
	// 4 -> Callee Idx Val
	// 5 -> Callee Ptr
	// 6 -> Empty

	uint32_t getTypeFeedbackVal() const;
	ObservedTest getTestFeedbackVal() const;
	int getSrcIdxVal() const;

	static L2Feedback create(uint32_t typeFeedbackVal, bool typeFeedback) {
		L2Feedback res;
		if (typeFeedback) {
			res.tag = 0;
			res.fVal.typeFeedbackVal = typeFeedbackVal;
		} else {
			res.tag = 4;
			res.fVal.srcIdx = typeFeedbackVal;
		}
		return res;
	}

	static L2Feedback create(ObservedValues* typeFeedbackPtr) {
		L2Feedback res;
		res.tag = 1;
		res.fVal.typeFeedbackPtr = typeFeedbackPtr;
		return res;
	}

	static L2Feedback create(ObservedTest testVal) {
		L2Feedback res;
		res.tag = 2;
		res.fVal.testVal = testVal;
		return res;
	}

	static L2Feedback create(Opcode * pc) {
		L2Feedback res;
		res.tag = 3;
		res.fPtr.pc = pc;
		return res;
	}

	// static L2Feedback create(unsigned srcIdx) {
	// 	L2Feedback res;
	// 	res.tag = 4;
	// 	res.fVal.srcIdx = srcIdx;
	// 	return res;
	// }

	static L2Feedback create(rir::Code * code, Opcode * pc) {
		L2Feedback res;
		res.tag = 5;
		res.fPtr.code = code;
		res.fPtr.pc = pc;
		return res;
	}

	static L2Feedback create() {
		L2Feedback res;
		res.tag = 6;
		return res;
	}

	inline bool operator==(const L2Feedback& other);

	void print(std::ostream& out, const int & space = 0) const;

private:
	int tag;
	FeedbackValue fVal;
	GenFeedbackHolder fPtr;

};

struct L2Function {
	Function * fun;
	L2Feedback * feedback;
};


#define L2_DISPATCH_MAGIC (unsigned)0X1e02ca11

#define GROWTH_RATE 5

#define FALLBACK_FN 0
#define FN_LIST 1
#define ENTRIES_SIZE 2

#define ASSERT_CONDITIONS 0


/*
 * Speculative Dispatcher
 *
 * SEXP count: 2
 *
 * [Idx 0]: (FALLBACK_FN) Fallback function to use in case no speculative context matches
 * [Idx 1]: (FN_LIST) Function list, each entry is a tuple containing a function and its associated speculative context
 *
 *
 */

#pragma pack(push)
#pragma pack(1)
struct L2Dispatch
    : public RirRuntimeObject<L2Dispatch, L2_DISPATCH_MAGIC> {
	// Constructor
	static L2Dispatch* create(Function* fallback, Protect & p) {
		size_t sz =
			sizeof(L2Dispatch) + (ENTRIES_SIZE * sizeof(SEXP));
		SEXP s;
		p(s = Rf_allocVector(EXTERNALSXP, sz));
		return new (INTEGER(s)) L2Dispatch(fallback);
	}

	// SLOT 0: FALLBACK_FN
	void setFallback(Function * f) {
		setEntry(FALLBACK_FN, f->container());
	}

	Function * getFallback() {
		SEXP fallbackContainer = getEntry(FALLBACK_FN);
		assert(Function::check(fallbackContainer));
		return Function::unpack(fallbackContainer);
	}

	// SLOT 1: FUNCTION_LIST
	void insert(Function * f);

	Function * getFunction(const int & idx) {
		assert(idx <= _last);
		SEXP FunctionList = getEntry(FN_LIST);
		SEXP fun = VECTOR_ELT(FunctionList, idx);
		return Function::unpack(fun);
	}


	Function * dispatch();

	// Genesis
	int functionListContainerSize() {
		return Rf_length(getEntry(FN_LIST));
	}

	void print(std::ostream& out, const int & space = 0);
  private:

	int _last = -1;
	Function * lastDispatch = nullptr;

	L2Dispatch() = delete;

	explicit L2Dispatch(Function* fallback);
	// Expands the function list by GROWTH_RATE
	void expandStorage();

};
#pragma pack(pop)
} // namespace rir
