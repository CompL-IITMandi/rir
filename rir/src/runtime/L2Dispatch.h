#pragma once
#include "Function.h"
#include "R/Protect.h"
namespace rir {

#define L2_DISPATCH_MAGIC (unsigned)0X1e02ca11

typedef SEXP L2DispatchEntry;

#define ENTRIES_SIZE 3
#define GROWTH_RATE 5

#define BCVEC 0
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

		static L2Dispatch* create(Function* genesis, const std::vector<ObservedValues> & TVals, const std::vector<ObservedValues*> & BCTFSlots, Protect & p) {
			size_t sz =
					sizeof(L2Dispatch) + (ENTRIES_SIZE * sizeof(L2DispatchEntry));
			SEXP s;
			p(s = Rf_allocVector(EXTERNALSXP, sz));
			return new (INTEGER(s)) L2Dispatch(genesis, TVals, BCTFSlots);
    	}

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
					// std::cout << "V1 HIT" << std::endl;
					return currFun;
				}
			}

			// std::cout << "V1 MISS" << std::endl;

			// Return the last inserted function.
			// BAD CASE
			return Function::unpack(VECTOR_ELT(functionVector, _last));
		}

		ObservedValues * getBCSlots() {
			SEXP BCVecContainer = getEntry(BCVEC);
			ObservedValues* * tmp = (ObservedValues* *) DATAPTR(BCVecContainer);
			return *tmp;
		}

		uint32_t getFeedbackAsUint(const rir::ObservedValues & v) {
			return *((uint32_t *) &v);
		}

		Function * V2Dispatch() {
			ObservedValues* observedTF = getBCSlots();
			SEXP functionVector = getEntry(FVEC);
			SEXP functionTFVector = getEntry(TVEC);
			// In this dispatch, only one type version is assumed so latest linked and available method is dispatched
			for (int i = _last; i >= 0; i--) {
				SEXP currFunHolder = VECTOR_ELT(functionVector, i);
				SEXP currTFHolder = VECTOR_ELT(functionTFVector, i);

				Function * currFun = Function::unpack(currFunHolder);

				ObservedValues* currTF = (ObservedValues *) DATAPTR(currTFHolder);

				bool match = true;

				// std::cout << "Checking Function " << i << "(" << currFun->disabled() << ")" << std::endl;

				for (unsigned int j = 0; j < _numSlots; j++) {
					// std::cout << "Slot[" << j << "]: " << getFeedbackAsUint(observedTF[j]) << ", " << getFeedbackAsUint(currTF[j]) << std::endl;
					if (getFeedbackAsUint(observedTF[j]) != getFeedbackAsUint(currTF[j])) match = false;
				}

				if (match && !currFun->disabled()) {
					// std::cout << "V2 HIT" << std::endl;
					return currFun;
				}
			}
			// std::cout << "V2 MISS" << std::endl;
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

		// V = 2
		void insert(Function* f, const std::vector<ObservedValues> & TVals) {
			// Expand the vector if capacity is full
			if (_last + 1 == capacity()) {
				expandStorage();
			}
			assert(_last + 1 < capacity());
			SEXP FunctionVector = getEntry(FVEC);
			SEXP TFVector       = getEntry(TVEC);

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
		}


		// V = 1
		void insert(Function* f) {
			// Expand the vector if capacity is full
			if (_last + 1 == capacity()) {
				expandStorage();
			}
			assert(_last + 1 < capacity());
			SEXP FunctionVector = getEntry(FVEC);

			_last++;
			SET_VECTOR_ELT(FunctionVector, _last, f->container());
		}

		unsigned int capacity() {
			return Rf_length(getEntry(FVEC));
		}

  private:
    L2Dispatch() = delete;

		explicit L2Dispatch(Function* genesis):
			RirRuntimeObject(sizeof(L2Dispatch),ENTRIES_SIZE),
			_versioning(1) {
				Protect protecc;
				// We leave the BCVector and TFVector to be null, we wont be using it
				SEXP FunctionVector;
				protecc(FunctionVector = Rf_allocVector(VECSXP, GROWTH_RATE));

				_last = 0;

				SET_VECTOR_ELT(FunctionVector, _last, genesis->container());

				// Store the vector in the GC area
				setEntry(FVEC, FunctionVector);
			}

		// BCTFSlots are the required bytecode type feedback slot pointers
		explicit L2Dispatch(Function* genesis, const std::vector<ObservedValues> & TVals, const std::vector<ObservedValues*> & BCTFSlots):
			RirRuntimeObject(sizeof(L2Dispatch),ENTRIES_SIZE),
			_versioning(2) {
				assert(TVals.size() == BCTFSlots.size());

				_numSlots = TVals.size();
				Protect protecc;
				//
				// 1. Populate BCVEC entries to point to ByteCode Locations
				//
				SEXP BCVector;
				// We make it a contigious big object, so we can directly index without VECTOR_ELT for each slot
				protecc(BCVector = Rf_allocVector(RAWSXP, _numSlots * sizeof(ObservedValues*)));
				ObservedValues* * tmp = (ObservedValues* *) DATAPTR(BCVector);

				// ObservedValues* workWith = *tmp;

				for (unsigned int i = 0; i < _numSlots; i++) {
					ObservedValues* ele = BCTFSlots[i];
					//
					// Note: Pointer-pointers are so weird
					//
					tmp[i] = ele;
				}

				// Store BCVEC in the GC area
				setEntry(BCVEC, BCVector);


				//
				// 2. Populate Function Vector and its corresponding TFVector
				//

				SEXP FunctionVector, TFVector;
				protecc(FunctionVector = Rf_allocVector(VECSXP, GROWTH_RATE));
				protecc(TFVector = Rf_allocVector(VECSXP, GROWTH_RATE));

				_last = 0;

				SET_VECTOR_ELT(FunctionVector, _last, genesis->container());

				// Create a store of ObservedValues for the TFValues
				SEXP store;
				// Note: instead of pointers, we store the values here instead
				protecc(store = Rf_allocVector(RAWSXP, _numSlots * sizeof(ObservedValues)));

				ObservedValues * tmp1 = (ObservedValues *) DATAPTR(store);

				for (unsigned int i = 0; i < _numSlots; i++) {
					tmp1[i] = TVals[i];
				}

				SET_VECTOR_ELT(TFVector, _last, store);


				// Store the vector in the GC area
				setEntry(FVEC, FunctionVector);

				setEntry(TVEC, TFVector);


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

    const unsigned _versioning;
    unsigned int _last;
	unsigned int _numSlots;
};
#pragma pack(pop)
} // namespace rir
