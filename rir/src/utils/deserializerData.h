#pragma once

#include "Rinternals.h"
#include "runtime/Context.h"

namespace rir {
    struct generalUtil {
        static void addSEXP(SEXP container, SEXP data, const int & index) {
            SET_VECTOR_ELT(container, index, data);
        }

        static SEXP getSEXP(SEXP container, const int & index) {
            return VECTOR_ELT(container, index);
        }

        static void addUnsignedLong(SEXP container, const unsigned long & data, const int & index) {
            SEXP store;
            PROTECT(store = Rf_allocVector(RAWSXP, sizeof(unsigned long)));
            unsigned long * tmp = (unsigned long *) DATAPTR(store);
            *tmp = data;
            SET_VECTOR_ELT(container, index, store);
            UNPROTECT(1);
        }

        static unsigned long getUnsignedLong(SEXP container, const int & index) {
            SEXP resContainer = getSEXP(container, index);
            assert(TYPEOF(resContainer) == RAWSXP);
            unsigned long* res = (unsigned long *) DATAPTR(resContainer);
            return *res;
        }

        static void addInt(SEXP container, const int & data, const int & index) {
            SEXP store;
            PROTECT(store = Rf_ScalarInteger(data));
            SET_VECTOR_ELT(container, index, store);
            UNPROTECT(1);
        }

        static int getInt(SEXP container, const int & index) {
            SEXP resContainer = getSEXP(container, index);
            assert(TYPEOF(resContainer) == INTSXP);
            return Rf_asInteger(resContainer);
        }

        static void addUint32t(SEXP container, const uint32_t & data, const int & index) {
            SEXP store;
            PROTECT(store = Rf_allocVector(RAWSXP, sizeof(uint32_t)));
            uint32_t * tmp = (uint32_t *) DATAPTR(store);
            *tmp = data;
            SET_VECTOR_ELT(container, index, store);
            UNPROTECT(1);
        }

        static uint32_t getUint32t(SEXP container, const int & index) {
            SEXP resContainer = getSEXP(container, index);
            assert(TYPEOF(resContainer) == RAWSXP);
            uint32_t* res = (uint32_t *) DATAPTR(resContainer);
            return *res;
        }

        static void printSpace(const int & size) {
            assert(size >= 0);
            for (int i = 0; i < size; i++) {
                std::cout << " ";
            }
        }

    };

    struct binaryUnit : generalUtil {
        // Vector: [EPOCH, REQMAP, TVDATA]

        //
        // 0: Epoch: Postfix UID of the filename, HAST_OFFSET_EPOCH.[bc|pool]
        //
        static void addEpoch(SEXP container, SEXP data) {
            assert(TYPEOF(data) == SYMSXP);
            addSEXP(container, data, 0);
        }
        static SEXP getEpoch(SEXP container) { return getSEXP(container, 0); }

        //
        // 1: ReqMap
        //
        static void addReqMap(SEXP container, SEXP data) { addSEXP(container, data, 1); }
        static SEXP getReqMap(SEXP container) { return getSEXP(container, 1); }

        //
        // 2: TVData: Optional
        //
        static void addTVData(SEXP container, SEXP data) { addSEXP(container, data, 2); }
        static SEXP getTVData(SEXP container) { return getSEXP(container, 2); }

        static void addTVData(SEXP container, std::vector<uint32_t> slotData) {
            SEXP store;
            PROTECT(store = Rf_allocVector(VECSXP, slotData.size()));
            int i = 0;
            for (auto & ele : slotData) {
                addUint32t(store, ele, i);
                i++;
            }
            addTVData(container, store);
            UNPROTECT(1);
        }


        static unsigned int getContainerSize() { return 3; }

        static void print(SEXP container, const unsigned int & space) {
            printSpace(space);
            std::cout << "├─(ENTRY 0, Epoch   ): " << CHAR(PRINTNAME(getEpoch(container))) << std::endl;

            printSpace(space);
            SEXP rMap = getReqMap(container);
            std::cout << "├─(ENTRY 1, ReqMap  ): (" << Rf_length(rMap) << "): [ ";
            for (int i = 0; i < Rf_length(rMap); i++) {
                std::cout << CHAR(PRINTNAME(VECTOR_ELT(rMap, i))) << " ";
            }
            std::cout << "]" << std::endl;



            if (getTVData(container) == R_NilValue) {
                printSpace(space);
                std::cout << "└─(ENTRY 2, TV Slots): NULL" << std::endl;
            } else {

                SEXP TVData = getTVData(container);

                printSpace(space);
                std::cout << "└─(ENTRY 2, TV Slots): [ ";

                for (int i = 0; i < Rf_length(TVData); i++) {
                    auto ele = getUint32t(TVData, i);
                    std::cout << ele << " ";
                }
                std::cout << "]" << std::endl;
            }
        }

    };

    struct contextUnit : generalUtil {
        // Vector: [context, VersionInt, TFSlots, binaryUnit, ...]

        //
        // 0: Context
        //
        static void addContext(SEXP container, SEXP data) { addSEXP(container, data, 0); }
        static SEXP getContext(SEXP container) { return getSEXP(container, 0); }

        static void addContext(SEXP container, const unsigned long & data) { addUnsignedLong(container, data, 0); }
        static unsigned long getContextAsUnsignedLong(SEXP container) { return getUnsignedLong(container, 0); }

        //
        // 1: Version
        //

        static void addVersioning(SEXP container, SEXP data) { addSEXP(container, data, 1); }
        static SEXP getVersioning(SEXP container) { return getSEXP(container, 1); }

        static void addVersioning(SEXP container, const int & data) { addInt(container, data, 1); }
        static int getVersioningAsInt(SEXP container) { return getInt(container, 1); }

        //
        // 2: TFSlots
        //

        static void addTFSlots(SEXP container, SEXP data) { addSEXP(container, data, 2); }
        static SEXP getTFSlots(SEXP container) { return getSEXP(container, 2); }

        static void addTFSlots(SEXP container, const std::vector<int> & data) {
            SEXP store;
            PROTECT(store = Rf_allocVector(VECSXP, data.size()));
            int i = 0;
            for (auto & ele : data) {
                addInt(store, ele, i);
                i++;
            }
            addTFSlots(container, store);
            UNPROTECT(1);
        }

        //
        // Other
        //

        static unsigned int getContainerSize(const int & n) { return reserved() + n; }

        static unsigned int reserved() { return 3; }

        static unsigned int binsStartingIndex() { return 3; }

        static unsigned int getNumBins(SEXP container) {
            assert(TYPEOF(container) == VECSXP);
            return Rf_length(container) - reserved();
        }

        //
        // Iterator
        // Callback: f(SEXP binaryUnit)
        //

        static void iterator(SEXP container, const std::function< void(SEXP) >& callback) {
            unsigned int n = Rf_length(container);
            for (unsigned int i = binsStartingIndex(); i < n; i++) {
                callback(getSEXP(container, i));
            }
        }

        static void print(SEXP container, const unsigned int & space) {
            printSpace(space);
            std::cout << "├─(ENTRY 0, Context   ): (" << getContextAsUnsignedLong(container) << ") " << rir::Context(getContextAsUnsignedLong(container)) << std::endl;

            printSpace(space);
            std::cout << "├─(ENTRY 1, Versioning): " << getVersioningAsInt(container) << std::endl;

            printSpace(space);
            SEXP tfData = getTFSlots(container);

            if (tfData == R_NilValue) {
                std::cout << "└─(ENTRY 2, TV Slots  ): [ ]" << std::endl;
            } else {
                std::cout << "└─(ENTRY 2, TV Slots  ): " << "[ ";
                for (int i = 0; i < Rf_length(tfData); i++) {
                    std::cout << Rf_asInteger(VECTOR_ELT(tfData, i)) << " ";
                }
                std::cout << "]" << std::endl;
            }

            auto numBin = getNumBins(container);
            int i = 1;

            iterator(container, [&] (SEXP binaryUnitContainer) {
                printSpace(space + 2);
                std::cout << "└─[Binary Unit]: " << i++ << "/" << numBin << std::endl;

                binaryUnit::print(binaryUnitContainer, space + 4);
            });


        }
    };

    struct offsetUnit : generalUtil {
        // offsetUnit: [Idx, Mask, contextUnit, contextUnit, ...]

        //
        // 0: Offset Index
        //
        static void addOffsetIdx(SEXP container, SEXP data) { addSEXP(container, data, 0); }
        static SEXP getOffsetIdx(SEXP container) { return getSEXP(container, 0); }

        static void addOffsetIdx(SEXP container, const int & data) { addInt(container, data, 0); }
        static int getOffsetIdxAsInt(SEXP container) { return getInt(container, 0); }

        //
        // 1: Context Mask
        //
        static void addMask(SEXP container, SEXP data) { addSEXP(container, data, 1); }
        static SEXP getMask(SEXP container) { return getSEXP(container, 1); }

        static void addMask(SEXP container, const unsigned long & data) { addUnsignedLong(container, data, 1); }
        static unsigned long getMaskAsUnsignedLong(SEXP container) { return getUnsignedLong(container, 1); }

        //
        // Other
        //

        static unsigned int getContainerSize(const int & n) { return reserved() + n; }

        static unsigned int reserved() { return 2; }

        static unsigned int contextsStartingIndex() { return 2; }

        static unsigned int getNumContexts(SEXP container) {
            assert(TYPEOF(container) == VECSXP);
            return Rf_length(container) - reserved();
        }

        //
        // Iterator
        // Callback: f(SEXP contextUnit)
        //

        static void iterator(SEXP container, const std::function< void(SEXP) >& callback) {
            unsigned int n = Rf_length(container);
            for (unsigned int i = contextsStartingIndex(); i < n; i++) {
                callback(getSEXP(container, i));
            }
        }

        static void print(SEXP container, const unsigned int & space) {

            printSpace(space);
            std::cout << "├─(ENTRY 0, OffsetIdx): " << getOffsetIdxAsInt(container) << std::endl;

            printSpace(space);
            std::cout << "└─(ENTRY 1, mask     ): (" << getMaskAsUnsignedLong(container) << ")" << rir::Context(getMaskAsUnsignedLong(container)) << std::endl;

            auto numCon = getNumContexts(container);
            int i = 1;

            iterator(container, [&] (SEXP contextUnitContainer) {
                printSpace(space + 2);
                std::cout << "└─[Context Unit]: " << i++ << "/" << numCon << std::endl;

                contextUnit::print(contextUnitContainer, space + 4);
            });
        }

    };

    struct deserializerData : generalUtil {
        // offsetUnit: [Hast, offsetUnit, offsetUnit, ...]

        //
        // 0: Hast
        //
        static void addHast(SEXP container, SEXP data) {
            assert(TYPEOF(data) == SYMSXP);
            addSEXP(container, data, 0);
        }
        static SEXP getHast(SEXP container) { return getSEXP(container, 0); }

        //
        // Other
        //

        static unsigned int getContainerSize(const int & n) { return reserved() + n; }

        static unsigned int reserved() { return 1; }

        static unsigned int offsetsStartingIndex() { return 1; }

        static unsigned int getNumOffsets(SEXP container) {
            assert(TYPEOF(container) == VECSXP);
            return Rf_length(container) - reserved();
        }

        //
        // Iterator
        // Callback: f(SEXP offsetUnit)
        //

        static void iterator(SEXP container, const std::function< void(SEXP) >& callback) {
            unsigned int n = Rf_length(container);
            for (unsigned int i = offsetsStartingIndex(); i < n; i++) {
                callback(getSEXP(container, i));
            }
        }

        //
        // callback(ddContainer, offsetUnitContainer, contextUnitContainer, binaryUnitContainer)
        //
        static void iterateOverUnits(SEXP ddContainer, const std::function< void(SEXP, SEXP, SEXP, SEXP) >& callback) {

            iterator(ddContainer, [&](SEXP offsetUnitContainer) {

                offsetUnit::iterator(offsetUnitContainer, [&] (SEXP contextUnitContainer) {

                    contextUnit::iterator(contextUnitContainer, [&](SEXP binaryUnitContainer) {

                        callback(ddContainer, offsetUnitContainer, contextUnitContainer, binaryUnitContainer);

                    });
                });

            });
        }

        static void print(SEXP container, const int & space) {
            printSpace(space);
            std::cout << "Deserializer Data: " << CHAR(PRINTNAME(getHast(container))) << std::endl;

            auto numOffsets = getNumOffsets(container);

            int i = 1;
            iterator(container, [&](SEXP offsetUnitData) {
                printSpace(space + 2);
                std::cout << "└─[Offset Unit]: " << i++ << "/" << numOffsets << std::endl;
                offsetUnit::print(offsetUnitData, space + 4);
            });
        }
    };
};;
