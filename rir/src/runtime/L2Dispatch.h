#pragma once
#include "Function.h"
#include "R/Protect.h"
namespace rir {

#define L2_DISPATCH_MAGIC (unsigned)0X1e02ca11

typedef SEXP L2DispatchEntry;

#define DEBUG_HIT_MISS 0
#define ENTRIES_SIZE 6
#define GROWTH_RATE 5

#define BCVEC 0
#define FVEC 1
#define TVEC 2
#define GENESIS 3
#define GVEC 4

#define BCGVEC 5
/*
 * A level 2 dispatcher for type versioning.
 *
 * SEXP count: 3
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

		Function * V1Dispatch();

		void disassemble(std::ostream& out) {
			// ObservedValues* observedTF = getBCSlots();
			SEXP functionVector = getEntry(FVEC);
			SEXP functionTFVector = getEntry(TVEC);
			out << "= L2: " << Function::unpack(getGenesisFunctionContainer())->context() << " =" << std::endl;
			for (unsigned int j = 0; j < _numSlots; j++) {
				out << "	SLOT(" << j << "): " << BCTFSlots[j] << "[" << getFeedbackAsUint(*BCTFSlots[j]) << "]" << std::endl;
				out << "		[ ";
				BCTFSlots[j]->print(out);
				out << "]" << std::endl;
			}

			// out << "= Genesis slot: " << Function::unpack(getGenesisFunctionContainer()) << std::endl;
			// // Function::unpack(getGenesisFunctionContainer())->disassemble(out);

			for (int i = _last; i >= 0; i--) {
				SEXP currFunHolder = VECTOR_ELT(functionVector, i);
				SEXP currTFHolder = VECTOR_ELT(functionTFVector, i);
				Function * currFun = Function::unpack(currFunHolder);
				ObservedValues* currTF = (ObservedValues *) DATAPTR(currTFHolder);

				out << "= L2 slot: " << i << "(" << currFun << ")" << std::endl;
				currFun->disassemble(out);
				for (unsigned int j = 0; j < _numSlots; j++) {
					bool match = true;
					out << "		[ ";
					currTF[j].print(out);
					out << "]" << std::endl;


					for (unsigned int j = 0; j < _numSlots; j++) {
						if (getFeedbackAsUint(*BCTFSlots[j]) != getFeedbackAsUint(currTF[j])) match = false;
					}
					if (match) {
						std::cout << "TF MATCHED" << std::endl;
					}

				}
			}
		}

		// ObservedValues * getBCSlots() {
		// 	SEXP BCVecContainer = getEntry(BCVEC);
		// 	ObservedValues* * tmp = (ObservedValues* *) DATAPTR(BCVecContainer);
		// 	return *tmp;
		// }

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
		void insert(Function* f, const std::vector<ObservedValues> & TVals, const std::vector<int> & GFBVals);

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
		explicit L2Dispatch(Function* genesis, const std::vector<ObservedValues*> & L2Slots, const std::vector<GenFeedbackHolder> & GenL2Slots);

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
				// Expand type
				oldVec = getEntry(TVEC);
				oldSize = Rf_length(oldVec);
				p(newVec = Rf_allocVector(VECSXP, oldSize + GROWTH_RATE));
				memcpy(DATAPTR(newVec), DATAPTR(oldVec), oldSize * sizeof(SEXP));

				setEntry(TVEC, newVec);

				// Expand general
				oldVec = getEntry(GVEC);
				oldSize = Rf_length(oldVec);
				p(newVec = Rf_allocVector(VECSXP, oldSize + GROWTH_RATE));
				memcpy(DATAPTR(newVec), DATAPTR(oldVec), oldSize * sizeof(SEXP));

				setEntry(GVEC, newVec);
			}

		}
    const unsigned _versioning;

    std::vector<ObservedValues*> BCTFSlots;

    int _last = -1;
	unsigned int _numSlots;
	unsigned int _numGenSlots;

};
#pragma pack(pop)
} // namespace rir
