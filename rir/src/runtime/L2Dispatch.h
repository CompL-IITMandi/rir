#pragma once
#include "Function.h"
#include "R/Protect.h"
namespace rir {

#define L2_DISPATCH_MAGIC (unsigned)0X1e02ca11

typedef SEXP L2DispatchEntry;

#define DEBUG_L2_CREATION 0


#define DEBUG_HIT_MISS 0
#define GROWTH_RATE 5

#define ENTRIES_SIZE 6
#define BCVEC 0
#define FVEC 1
#define TVEC 2
#define GVEC 3
#define GENESIS 4
#define GFBVEC 5


/*
 * A level 2 dispatcher for type versioning.
 *
 * // TODO, UPDATE THE DESCRIPTIONS
 *
 * SEXP count: 6
 *
 * [Idx 0]: (aka CVector) (RAWSXP holding 'n' ObservedValues*) These are pointers to TypeFeedbackSlots in the Bytecode
 * [Idx 1]: (aka FunctionVector) Vector of (RAWSXP holding Function*)
 * [Idx 2]: (aka TFVector) Vector of (RAWSXP holding ObservedValues*)
 * [Idx 3]: [Used in the case if V = 2]Gensis fallback function, initially null. But if JIT compiles something, that is put here.
 *
 * Size of vectors at [Idx 1] and [Idx 2] is always the same and they are meant to be used together
 * in type versioning
 *
 */

#pragma pack(push)
#pragma pack(1)
struct L2Dispatch
    : public RirRuntimeObject<L2Dispatch, L2_DISPATCH_MAGIC> {

		//
		// Constructor For V = 2
		//
		static L2Dispatch* create(Function* genesis, const std::vector<ObservedValues*> & BCTFSlots, const std::vector<GenFeedbackHolder> & GENSlots, Protect & p) {
			size_t sz =
					sizeof(L2Dispatch) + (ENTRIES_SIZE * sizeof(L2DispatchEntry));
			SEXP s;
			p(s = Rf_allocVector(EXTERNALSXP, sz));
			return new (INTEGER(s)) L2Dispatch(genesis, BCTFSlots, GENSlots);
    	}

		//
		// Constructor For V = 1
		//
		static L2Dispatch* create(Function* genesis, Protect & p) {
			size_t sz =
					sizeof(L2Dispatch) + (ENTRIES_SIZE * sizeof(L2DispatchEntry));
			SEXP s;
			p(s = Rf_allocVector(EXTERNALSXP, sz));
			return new (INTEGER(s)) L2Dispatch(genesis);
    	}

		SEXP getGenesisFunctionContainer() {
			return getEntry(GENESIS);
		}

		//
		// V = 1 dispatch, Here we only dispatch to the latest unlocked binary and move towards more generic ones if we deopt
		//
		Function * V1Dispatch() {
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

		void disassemble(std::ostream& out) {
			ObservedValues* observedTF = getBCSlots();
			SEXP functionVector = getEntry(FVEC);
			SEXP functionTFVector = getEntry(TVEC);
			out << "= L2 dispatch Start =" << std::endl;

			out << "= Genesis slot: " << Function::unpack(getGenesisFunctionContainer()) << std::endl;
			Function::unpack(getGenesisFunctionContainer())->disassemble(out);

			for (int i = _last; i >= 0; i--) {
				SEXP currFunHolder = VECTOR_ELT(functionVector, i);
				SEXP currTFHolder = VECTOR_ELT(functionTFVector, i);
				Function * currFun = Function::unpack(currFunHolder);
				ObservedValues* currTF = (ObservedValues *) DATAPTR(currTFHolder);

				out << "= L2 slot: " << i << "(" << currFun << ")" << std::endl;
				for (unsigned int j = 0; j < _numSlots; j++) {
					std::cout << "Slot[" << j << "]: " << getFeedbackAsUint(observedTF[j]) << ", " << getFeedbackAsUint(currTF[j]) << std::endl;
				}
				currFun->disassemble(out);
			}
		}

		ObservedValues * getBCSlots() {
			SEXP BCVecContainer = getEntry(BCVEC);
			ObservedValues* * tmp = (ObservedValues* *) DATAPTR(BCVecContainer);
			return *tmp;
		}

		GenFeedbackHolder * getGFBSlots() {
			SEXP GFBVecContainer = getEntry(GFBVEC);
			GenFeedbackHolder * tmp = (GenFeedbackHolder *) DATAPTR(GFBVecContainer);
			return tmp;
		}

		uint32_t getFeedbackAsUint(const rir::ObservedValues & v) {
			return *((uint32_t *) &v);
		}

		//
		// V = 2 dispatch, Here we dispatch to the latest linked and avaialble (non-disabled) binary for a given type feedback context.
		// We fallback to genesis in case nothing is found because randomly dispatching to binary may make that type version to be
		// unnecessarity disabled, so for brevity we only dispatch when things are as expected.
		//

		Function * V2Dispatch();

		Function * dispatch() {
			// If nothing exists return genesis
			if (_last == -1) {
				return Function::unpack(getGenesisFunctionContainer());
			}
			if (_versioning == 1) {
				return V1Dispatch();
			} else {
				return V2Dispatch();
			}
		}

		// V = 2
		void insert(Function* f, const std::vector<ObservedValues> & TVals, const std::vector<int> & GFBVals) {
			// Expand the vector if capacity is full
			if (_last + 1 == capacity()) {
				expandStorage();
			}
			assert(_last + 1 < capacity());
			SEXP FunctionVector = getEntry(FVEC);
			SEXP TFVector       = getEntry(TVEC);
			SEXP GFBVector      = getEntry(GVEC);

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



			// Create a store of int for the GFBVals
			// Note: instead of pointers, we store the values here instead
			protecc(store = Rf_allocVector(RAWSXP, _numGenSlots * sizeof(int)));

			int * tmp1 = (int *) DATAPTR(store);

			for (unsigned int i = 0; i < _numGenSlots; i++) {
				tmp1[i] = GFBVals[i];
			}

			SET_VECTOR_ELT(GFBVector, _last, store);

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

		// Genesis
		// Update the genesis to point to JIT compiled function.
		void insertGenesis(Function* f) {
			setEntry(GENESIS, f->container());
		}

		int capacity() {
			return Rf_length(getEntry(FVEC));
		}

		int entries() {
			return _last + 1;
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

				_last = -1;

				insertGenesis(genesis);

				// Store the vector in the GC area
				setEntry(FVEC, FunctionVector);
			}

		// BCTFSlots are the required bytecode type feedback slot pointers
		explicit L2Dispatch(Function* genesis, const std::vector<ObservedValues*> & BCTFSlots, const std::vector<GenFeedbackHolder> & GENSlots):
			RirRuntimeObject(sizeof(L2Dispatch),ENTRIES_SIZE),
			_versioning(2) {

				_numSlots = BCTFSlots.size();
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

				SEXP FunctionVector, TFVector, GVector;
				protecc(FunctionVector = Rf_allocVector(VECSXP, GROWTH_RATE));
				protecc(TFVector = Rf_allocVector(VECSXP, GROWTH_RATE));
				protecc(GVector = Rf_allocVector(VECSXP, GROWTH_RATE));

				_last = -1;

				insertGenesis(genesis);

				// Store the vector in the GC area
				setEntry(FVEC, FunctionVector);
				setEntry(TVEC, TFVector);
				setEntry(GVEC, GVector);


				_numGenSlots = GENSlots.size();
				//
				// 3. Populate General feedback locations
				//
				SEXP GFBVector;
				// We make it a contigious big object, so we can directly index without VECTOR_ELT for each slot
				protecc(GFBVector = Rf_allocVector(RAWSXP, _numGenSlots * sizeof(GenFeedbackHolder)));
				GenFeedbackHolder * tmp1 = (GenFeedbackHolder *) DATAPTR(GFBVector);

				for (unsigned int i = 0; i < _numGenSlots; i++) {
					//
					// Note: Pointer-pointers are so weird
					//
					tmp1[i] = GENSlots[i];
				}

				// Store the General Feedback into GC
				setEntry(GFBVEC, GFBVector);


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


				oldVec = getEntry(GVEC);
				oldSize = Rf_length(oldVec);
				p(newVec = Rf_allocVector(VECSXP, oldSize + GROWTH_RATE));
				memcpy(DATAPTR(newVec), DATAPTR(oldVec), oldSize * sizeof(SEXP));

				setEntry(GVEC, newVec);
			}

		}

    const unsigned _versioning;
    int _last = -1;
	unsigned int _numSlots;
	unsigned int _numGenSlots;
};
#pragma pack(pop)
} // namespace rir
