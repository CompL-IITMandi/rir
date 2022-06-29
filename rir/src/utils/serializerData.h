#pragma once

#include "R/r.h"
#include "R/Protect.h"
#include "ir/BC.h"
#include "utils/UMap.h"

#include <fstream>

#define PRINT_EXTENDED_CHILDREN 0

namespace rir {

    class containerDataAbstraction {
        public:
            SEXP container;

            inline void printSpace(int size) {
                assert(size >= 0);
                for (int i = 0; i < size; i++) {
                    std::cout << " ";
                }
            }

            void addFS(rir::FunctionSignature fs, int index) {
                SEXP store;
                PROTECT(store = Rf_allocVector(RAWSXP, sizeof(fs)));
                rir::FunctionSignature * tmp = (rir::FunctionSignature *) DATAPTR(store);
                #if defined(__GNUC__) || defined(__GNUG__)
                    #pragma GCC diagnostic push
                    #pragma GCC diagnostic ignored "-Wclass-memaccess"
                #endif

                memcpy(tmp, &fs, sizeof(fs));

                #if defined(__clang__)
                    #pragma GCC diagnostic pop
                #endif

                SET_VECTOR_ELT(container, index, store);
                UNPROTECT(1);
            }

            rir::FunctionSignature getFS(int index) {
                SEXP dataContainer = VECTOR_ELT(container, index);
                rir::FunctionSignature* res = (rir::FunctionSignature *) DATAPTR(dataContainer);
                return *res;
            }

            void addSizeT(size_t data, int index) {
                SEXP store;
                PROTECT(store = Rf_allocVector(RAWSXP, sizeof(size_t)));
                size_t * tmp = (size_t *) DATAPTR(store);
                *tmp = data;
                SET_VECTOR_ELT(container, index, store);
                UNPROTECT(1);
            }

            size_t getSizeT(int index) {
                SEXP dataContainer = VECTOR_ELT(container, index);
                size_t* res = (size_t *) DATAPTR(dataContainer);
                return *res;
            }

            void addInt(int data, int index) {
                SEXP store;
                PROTECT(store = Rf_allocVector(RAWSXP, sizeof(int)));
                int * tmp = (int *) DATAPTR(store);
                *tmp = data;
                SET_VECTOR_ELT(container, index, store);
                UNPROTECT(1);
            }

            int getInt(int index) {
                SEXP dataContainer = VECTOR_ELT(container, index);
                int* res = (int *) DATAPTR(dataContainer);
                return *res;
            }

            void addUnsigned(unsigned data, int index) {
                SEXP store;
                PROTECT(store = Rf_allocVector(RAWSXP, sizeof(unsigned)));
                unsigned * tmp = (unsigned *) DATAPTR(store);
                *tmp = data;
                SET_VECTOR_ELT(container, index, store);
                UNPROTECT(1);
            }

            unsigned getUnsigned(int index) {
                SEXP dataContainer = VECTOR_ELT(container, index);
                unsigned* res = (unsigned *) DATAPTR(dataContainer);
                return *res;
            }

            void addUnsignedLong(unsigned long data, int index) {
                SEXP store;
                PROTECT(store = Rf_allocVector(RAWSXP, sizeof(unsigned long)));
                unsigned long * tmp = (unsigned long *) DATAPTR(store);
                *tmp = data;
                SET_VECTOR_ELT(container, index, store);
                UNPROTECT(1);
            }

            unsigned long getUnsignedLong(int index) {
                SEXP dataContainer = VECTOR_ELT(container, index);
                unsigned long* res = (unsigned long *) DATAPTR(dataContainer);
                return *res;
            }

            void addString(std::string data, int index) {
                SEXP store;
                PROTECT(store = Rf_mkString(data.c_str()));
                SET_VECTOR_ELT(container, index, store);
                UNPROTECT(1);
            }

            std::string getString(int index) {
                SEXP dataContainer = VECTOR_ELT(container, index);
                return std::string(CHAR(STRING_ELT(dataContainer, 0)));
            }
    };

    class contextData : containerDataAbstraction {
        // {
        // 0 (unsigned long) Context
        // 1 (rir::FunctionSignature) Function Signature
        // 2 (SEXP) Function Names
        // 3 (SEXP) Function Src
        // 4 (SEXP) Function Arglist Order
        // 5 (SEXP) Children Data,
        // 6 (SEXP) reqMapForCompilation
        // }
        public:
            SEXP getContainer() {
                return container;
            }
            explicit contextData(SEXP c) {
                container = c;
            }

            // ENTRY 0: Con
            void addContext(unsigned long data) {
                addUnsignedLong(data, 0);
            }

            unsigned long getContext() {
                return getUnsignedLong(0);
            }

            // ENTRY 1: Function Signature
            void addFunctionSignature(rir::FunctionSignature fs) {
                addFS(fs, 1);
            }

            rir::FunctionSignature getFunctionSignature() {
                return getFS(1);
            }

            // ENTRY 2: Function Names
            void addFNames(SEXP data) {
                SET_VECTOR_ELT(container, 2, data);
            }

            SEXP getFNames() {
                return VECTOR_ELT(container, 2);
            }

            // ENTRY 3: Function Src
            void addFSrc(SEXP data) {
                SET_VECTOR_ELT(container, 3, data);
            }

            SEXP getFSrc() {
                return VECTOR_ELT(container, 3);
            }

            // ENTRY 4: Function Arglist Order
            void addFArg(SEXP data) {
                SET_VECTOR_ELT(container, 4, data);
            }

            SEXP getFArg() {
                return VECTOR_ELT(container, 4);
            }

            // ENTRY 7: childrenData
            void addFChildren(SEXP data) {
                SET_VECTOR_ELT(container, 5, data);
            }

            SEXP getFChildren() {
                return VECTOR_ELT(container, 5);
            }

            // ENTRY 8: reqMap
            void addReqMapForCompilation(SEXP data) {
                SET_VECTOR_ELT(container, 6, data);
            }

            SEXP getReqMapAsVector() {
                SEXP rMap = VECTOR_ELT(container, 6);
                return rMap;
            }

            void removeEleFromReqMap(SEXP ele) {
                auto rData = getReqMapAsVector();
                int containsElement = -1;
                std::string dep(CHAR(PRINTNAME(ele)));
                for (int i = 0; i < Rf_length(rData); i++) {
                    SEXP e = VECTOR_ELT(rData, i);
                    std::string curr(CHAR(PRINTNAME(e)));
                    if (curr.find(dep) != std::string::npos) {
                        containsElement = i;
                    }
                }

                if (containsElement != -1) {
                    SEXP newVec;
                    PROTECT(newVec = Rf_allocVector(VECSXP, Rf_length(rData) - 1));

                    int newVecIdx = 0;
                    for (int i = 0; i < Rf_length(rData); i++) {
                        SEXP e = VECTOR_ELT(rData, i);
                        if (i != containsElement) {
                            SET_VECTOR_ELT(newVec, newVecIdx++, e);
                        }
                    }

                    SET_VECTOR_ELT(container, 8, newVec);
                    UNPROTECT(1);

                }
            }


            void print() {
                print(0);
            }

            void print(int space) {
                printSpace(space);
                std::cout << "context: " << rir::Context(getContext()) << std::endl;
                space += 2;
                printSpace(space);
                std::cout << "ENTRY(0)[context]: " << getContext() << std::endl;
                printSpace(space);
                rir::FunctionSignature fs = getFunctionSignature();
                std::cout << "ENTRY(1)[Function Signature]: " << (int)fs.envCreation << ", " << (int)fs.optimization << ", " <<  fs.numArguments << ", " << fs.hasDotsFormals << ", " << fs.hasDefaultArgs << ", " << fs.dotsPosition << std::endl;
                printSpace(space);
                std::cout << "ENTRY(2)[Function names]: [ ";
                auto fNames = getFNames();
                for (int i = 0; i < Rf_length(fNames); i++) {
                    auto c = VECTOR_ELT(fNames, i);
                    std::cout << CHAR(STRING_ELT(c, 0)) << " ";
                }
                std::cout << "]" << std::endl;

                printSpace(space);
                std::cout << "ENTRY(3)[Function Src]: [ ";
                auto fSrc = getFSrc();
                for (int i = 0; i < Rf_length(fSrc); i++) {
                    auto c = VECTOR_ELT(fSrc, i);
                    if (c != R_NilValue) {
                        std::cout << "(" << CHAR(PRINTNAME(VECTOR_ELT(c, 0))) << "," << *INTEGER(VECTOR_ELT(c, 1)) << ") ";
                    } else {
                        std::cout << "(SRC_NULL) ";
                    }
                }
                std::cout << "]" << std::endl;

                printSpace(space);
                std::cout << "ENTRY(4)[Function Arglist Order]: [ ";
                auto fArg = getFArg();
                for (int i = 0; i < Rf_length(fArg); i++) {
                    auto c = VECTOR_ELT(fArg, i);
                    std::cout << TYPEOF(c) << " ";
                }
                std::cout << "]" << std::endl;

                printSpace(space);
                #if PRINT_EXTENDED_CHILDREN == 1
                std::cout << "ENTRY(5)[children Data]" << std::endl;
                #else
                std::cout << "ENTRY(5)[children Data]: ";
                #endif
                auto fChildren = getFChildren();
                for (int i = 0; i < Rf_length(fChildren); i++) {
                    auto cVector = VECTOR_ELT(fChildren, i);

                    #if PRINT_EXTENDED_CHILDREN == 1
                    printSpace(space);
                    auto handle = std::string(CHAR(STRING_ELT(VECTOR_ELT(fNames, i), 0)));
                    std::cout << handle << " : [ ";
                    #else
                    std::cout << "(" << Rf_length(cVector) << ")" << "[ ";
                    #endif

                    for (int j = 0; j < Rf_length(cVector); j++) {
                        auto d = VECTOR_ELT(cVector, j);

                        #if PRINT_EXTENDED_CHILDREN == 1
                        auto handleC = std::string(CHAR(STRING_ELT(VECTOR_ELT(fNames, Rf_asInteger(d)), 0)));
                        std::cout << handleC << " ";
                        #else
                        std::cout << Rf_asInteger(d) << " ";
                        #endif
                    }
                    std::cout << "] ";
                }

                #if PRINT_EXTENDED_CHILDREN == 0
                std::cout<< std::endl;
                #endif

                auto rData = getReqMapAsVector();
                printSpace(space);
                std::cout << "ENTRY(6)[reqMapForCompilation]: <";
                for (int i = 0; i < Rf_length(rData); i++) {
                    SEXP ele = VECTOR_ELT(rData, i);
                    std::cout << CHAR(PRINTNAME(ele)) << " ";
                }
                std::cout << ">" << std::endl;
            }
    };

    class serializerData : containerDataAbstraction {
        // 0 -> hast
        // 1 -> name
        // 2 -> envData { context: contextData }

        public:
            SEXP getContainer() {
                return container;
            }

            explicit serializerData(SEXP c) {
                container = c;
            }

            serializerData(SEXP c, SEXP hastSym, const std::string & name) {
                container = c;
                SET_VECTOR_ELT(container, 0, hastSym);
                addString(name, 1);
                SEXP envObj;
                PROTECT(envObj = R_NewEnv(R_EmptyEnv,0,0));
                SET_VECTOR_ELT(container, 2, envObj);
                UNPROTECT(1);
            }

            void updateContainer(SEXP c) {
                container = c;
            }

            void addContextData(SEXP cData, int offsetIndex, std::string context) {
                SEXP symSxp = Rf_install(context.c_str());
                SEXP mainMap = VECTOR_ELT(container, 2);
                SEXP offsetSym = Rf_install(std::to_string(offsetIndex).c_str());
                if (Rf_findVarInFrame(mainMap, offsetSym) == R_UnboundValue) {
                    SEXP envObj;
                    PROTECT(envObj = R_NewEnv(R_EmptyEnv,0,0));
                    Rf_defineVar(offsetSym, envObj, mainMap);
                    UNPROTECT(1);
                }
                SEXP offsetMap = Rf_findVarInFrame(mainMap, offsetSym);

                if (offsetIndex == 0) {
                    // Get rid of self dependencies, may happen in case of recursive functions
                    std::stringstream ss;
                    ss << CHAR(PRINTNAME(getHastData())) << "_" << context;
                    contextData conDataHandler(cData);
                    conDataHandler.removeEleFromReqMap(Rf_install(ss.str().c_str()));
                }


                Rf_defineVar(symSxp, cData, offsetMap);
            }

            std::string getNameData() {
                return getString(1);
            }

            SEXP getHastData() {
                return VECTOR_ELT(container, 0);
            }

            SEXP getContextMap() {
                return VECTOR_ELT(container, 2);
            }

            static void iterateOverOffsets(SEXP mainMap, const std::function< void(SEXP, SEXP) >& callback) {

                SEXP offsetBindings = R_lsInternal(mainMap, (Rboolean) false);
                for (int i = 0; i < Rf_length(offsetBindings); i++) {
                    SEXP offsetKeySym = Rf_install(CHAR(STRING_ELT(offsetBindings, i)));
                    SEXP offsetMap = Rf_findVarInFrame(mainMap, offsetKeySym);
                    callback(offsetKeySym, offsetMap);
                }
            }

            static void iterateOverContexts(SEXP offsetMap, const std::function< void(SEXP, SEXP) >& callback) {
                SEXP maskSym = Rf_install("mask");
                SEXP contextBindings = R_lsInternal(offsetMap, (Rboolean) false);
                for (int i = 0; i < Rf_length(contextBindings); i++) {
                    SEXP contextKeySym = Rf_install(CHAR(STRING_ELT(contextBindings, i)));
                    if (contextKeySym == maskSym) {
                        continue;
                    }
                    SEXP contextData = Rf_findVarInFrame(offsetMap, contextKeySym);
                    callback(contextKeySym, contextData);
                }
            }


            void print() {
                print(0);
            }

            void print(int space) {
                printSpace(space);
                std::cout << "serializerData: " << std::endl;
                space += 2;
                printSpace(space);
                std::cout << "ENTRY(-3): " << CHAR(PRINTNAME(getHastData())) << std::endl;
                printSpace(space);
                std::cout << "ENTRY(-2): " << getNameData() << std::endl;


                SEXP mainMap = VECTOR_ELT(container, 2);
                space += 2;

                iterateOverOffsets(mainMap, [&] (SEXP offsetSymbol, SEXP offsetEnv) {
                    printSpace(space);
                    std::cout << "offset: " << CHAR(PRINTNAME(offsetSymbol)) << std::endl;
                    iterateOverContexts(offsetEnv, [&] (SEXP contextSym, SEXP cData) {
                        contextData c(cData);
                        c.print(space);
                    });
                });
            }
    };


    class poolSerializeAndDeserialize {
        public:
        static bool serializePools(const std::string & path, SEXP cPool, SEXP sPool) {
            // Create a container
            SEXP poolsContainer;
            PROTECT(poolsContainer = Rf_allocVector(VECSXP, 2));

            SET_VECTOR_ELT(poolsContainer, 0, cPool);
            SET_VECTOR_ELT(poolsContainer, 1, sPool);

            // Write pools to file
            R_outpstream_st outputStream;
            FILE *fptr;
            fptr = fopen(path.c_str(),"w");
            if (!fptr) {
                return false;
            }
            R_InitFileOutPStream(&outputStream,fptr,R_pstream_binary_format, 0, NULL, R_NilValue);
            R_Serialize(poolsContainer, &outputStream);
            fclose(fptr);

            UNPROTECT(1);

            return true;
        }
    };
};;
