#pragma once
#include "Function.h"
#include "R/Protect.h"
namespace rir {

#define L2_DISPATCH_MAGIC (unsigned)0X1e02ca11

typedef SEXP L2DispatchEntry;

#define ENTRIES_SIZE 3
#define GROWTH_RATE 5

#define CVEC 0
#define FVEC 1
#define TVEC 2
/*
 * A level 2 dispatcher for type versioning.
 *
 * SEXP count: 3
 *
 * [Idx 0]: (aka CVector) (RAWSXP holding 'n' ObservedValues*) These are pointers to TypeFeedbackSlots in the Bytecode
 * [Idx 1]: (aka FunctionVector) Vector of (RAWSXP holding Function*)
 * [Idx 2]: (aka TFVector) Vector of (RAWSXP holding ObservedValues*)
 *
 * Size of vectors at [Idx 1] and [Idx 2] is always the same and they are meant to be used together
 * in type versioning
 *
 */

#pragma pack(push)
#pragma pack(1)
struct L2Dispatch
    : public RirRuntimeObject<L2Dispatch, L2_DISPATCH_MAGIC> {

		static L2Dispatch* create(Function* genesis, Protect & p) {
			size_t sz =
					sizeof(L2Dispatch) + (ENTRIES_SIZE * sizeof(L2DispatchEntry));
			SEXP s;
			p(s = Rf_allocVector(EXTERNALSXP, sz));
			return new (INTEGER(s)) L2Dispatch(genesis);
    	}


		Function * V1Dispatch() {
			SEXP functionVector = getEntry(FVEC);
			// In this dispatch, only one type version is assumed so latest linked and available method is dispatched
			for (int i = _last; i >= 0; i--) {
				SEXP currFunHolder = VECTOR_ELT(functionVector, i);
				Function * currFun = Function::unpack(currFunHolder);
				if (!currFun->disabled()) {
					return currFun;
				}
			}

			// Return the last inserted function.
			// BAD CASE
			return Function::unpack(VECTOR_ELT(functionVector, _last));
		}

		Function * V2Dispatch() {
			//
			// TODO
			//
			// Dispatch to latest available TV function, if this fails fallback to V1Dispatch
			return V1Dispatch();
		}

		Function * dispatch() {
			if (_versioning == 1) {
				return V1Dispatch();
			} else {
				return V2Dispatch();
			}
		}

		// V = 1
		void insert(Function* f) {
			// Expand the vector if capacity is full
			if (_last + 1 == capacity()) {
				expandStorage();
			}
			assert(_last + 1 < capacity());
			SEXP FunctionVector = getEntry(1);

			_last++;
			SET_VECTOR_ELT(FunctionVector, _last, f->container());
		}

		void expandStorage() {
			rir::Protect p;

			SEXP oldVec = getEntry(FVEC);
			int oldSize = Rf_length(oldVec);
			SEXP newVec;
			p(newVec = Rf_allocVector(VECSXP, oldSize + GROWTH_RATE));
			memcpy(DATAPTR(newVec), DATAPTR(oldVec), oldSize * sizeof(SEXP));

			setEntry(FVEC, newVec);

			// If versioning is 2, expand the TFVector aswell
			if (_versioning == 2) {
				oldVec = getEntry(TVEC);
				oldSize = Rf_length(oldVec);
				p(newVec = Rf_allocVector(VECSXP, oldSize + GROWTH_RATE));
				memcpy(DATAPTR(newVec), DATAPTR(oldVec), oldSize * sizeof(SEXP));

				setEntry(TVEC, newVec);
			}

		}

		unsigned int capacity() {
			return Rf_length(getEntry(FVEC));
		}

  private:
    L2Dispatch() = delete;

		explicit L2Dispatch(Function* genesis):
			RirRuntimeObject(sizeof(L2Dispatch),ENTRIES_SIZE),
			_versioning(1) {
				// We leave the CVector and TFVector to be null, we wont be using it
				SEXP FunctionVector;
				PROTECT(FunctionVector = Rf_allocVector(VECSXP, GROWTH_RATE));

				_last = 0;

				SET_VECTOR_ELT(FunctionVector, 0, genesis->container());

				// Store the vector in the GC area
				setEntry(FVEC, FunctionVector);

				UNPROTECT(1);
			}

    const unsigned _versioning;
    unsigned int _last;
};
#pragma pack(pop)
} // namespace rir
