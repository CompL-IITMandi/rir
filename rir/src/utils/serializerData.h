#pragma once

#include "R/r.h"
#include "R/Protect.h"
#include "bc/BC.h"
#include "utils/REnvHandler.h"

#include "runtime/TypeFeedback.h"
#include <fstream>

#include "utils/Debug.h"
#define PRINT_EXTENDED_CHILDREN 0

#include <chrono>
namespace rir {

    static void addSEXP(SEXP container, SEXP data, const int & index) {
        SET_VECTOR_ELT(container, index, data);
    }

    static SEXP getSEXP(SEXP container, const int & index) {
        return VECTOR_ELT(container, index);
    }

    struct speculativeContextElement {
        // 0 (SEXP_int) TAG (0: TypeFeedback, 1: TestFeedback, 2: Callee)
        // 1 (SEXP_rawsxp) Pointer to memory address where the slot is located (just here to make marking points of deopt easier while lowering)
        // 2 (SEXP_bool) Point of Deopt
        // 3 (SEXP_rawsxp | SEXP_symsxp) (0: uint32_t, 1: uint32_t, 2: SYMSXP)
        static unsigned getStorageSize() {
            return 4;
        }

        // ENTRY 0: Tag
        static void addTag(SEXP container, const int & data) {
            assert(data < 3);
            addTToContainer<int>(container, 0, data);
        }

        static int getTag(SEXP container) {
            auto res = getTFromContainer<int>(container, 0);
            assert(res < 3);
            return res;
        }

        // ENTRY 1: PC*
        static void addPC(SEXP container, uintptr_t pc) {
            addTToContainer<uintptr_t>(container, 1, pc);
        }

        static uintptr_t getPC(SEXP container) {
            if (getSEXP(container, 1) == R_NilValue) return 0;
            return getTFromContainer<uintptr_t>(container, 1);
        }

        // ENTRY 2: Point Of Deopt?
        static void addPOD(SEXP container, bool pod) {
            addTToContainer<bool>(container, 2, pod);
        }

        static bool getPOD(SEXP container) {
            return getTFromContainer<bool>(container, 2);
        }

        // ENTRY 3:
        static void addVal(SEXP container, uint32_t data) {
            addTToContainer<uint32_t>(container, 3, data);
        }

        static void addVal(SEXP container, SEXP data) {
            addSEXP(container, data, 3);
        }

        static uint32_t getValUint(SEXP container) {
            return getTFromContainer<uint32_t>(container, 3);
        }

        static SEXP getValSEXP(SEXP container) {
            return getSEXP(container, 3);
        }

        static void print(SEXP container) {
            std::cout << "[";
            switch(getTag(container)) {
                case 0:
                std::cout << "Type,"; break;
                case 1:
                std::cout << "Test,"; break;
                case 2:
                std::cout << "Call,"; break;
                default:
                std::cout << "ERR!,"; break;
            }
            // std::cout << getPC(container) << ",";
            std::cout << (getPOD(container) ? "D" : "U") << ",";
            switch(getTag(container)) {
                case 0:
                std::cout << getValUint(container); break;
                case 1:
                std::cout << getValUint(container); break;
                case 2: {
                    SEXP val = getValSEXP(container);
                    if (TYPEOF(val) == INTSXP) {
                        if (INTEGER(val)[0] == -1) {
                            std::cout << "NonHast";
                        } else if (INTEGER(val)[0] == -2) {
                            std::cout << "NonRirClos";
                        } else if (INTEGER(val)[0] == -3) {
                            std::cout << "NonMono";
                        } else {
                            std::cout << "Special(" << INTEGER(val)[0] << ")";
                        }
                    } else {
                        assert(TYPEOF(val) == SYMSXP);
                        std::cout << CHAR(PRINTNAME(val));
                    }
                    break;

                }
                default:
                std::cout << "ERR!"; break;
            }
            std::cout << "]";
        }
    };

    class SerializedPool {
        // 0 (rir::FunctionSignature) Function Signature
        // 1 (SEXP) Function Names
        // 2 (SEXP) Function Src
        // 3 (SEXP) Function Arglist Order
        // 4 (SEXP) Children Data,
        // 5 (SEXP) cPool
        // 6 (SEXP) sPool
        // 7 (SEXP) poolEpoch

    public:

        // Misc functions
        static unsigned getStorageSize() {
            return 8;
        }



        // ENTRY 0: Function Signature
        static void addFS(SEXP container, const rir::FunctionSignature & fs) {
            rir::Protect protecc;
            SEXP fsContainer;
            protecc(fsContainer = Rf_allocVector(VECSXP, 6));

            addTToContainer<rir::FunctionSignature::Environment>(fsContainer, 0, fs.envCreation);
            addTToContainer<rir::FunctionSignature::OptimizationLevel>(fsContainer, 1, fs.optimization);
            addTToContainer<unsigned>(fsContainer, 2, fs.numArguments);
            addTToContainer<bool>(fsContainer, 3, fs.hasDotsFormals);
            addTToContainer<bool>(fsContainer, 4, fs.hasDefaultArgs);
            addTToContainer<size_t>(fsContainer, 5, fs.dotsPosition);

            SET_VECTOR_ELT(container, 0, fsContainer);
        }

        static rir::FunctionSignature getFS(SEXP container) {
            SEXP dataContainer = VECTOR_ELT(container, 0);

            rir::FunctionSignature res(
                getTFromContainer<rir::FunctionSignature::Environment>(dataContainer, 0),
                getTFromContainer<rir::FunctionSignature::OptimizationLevel>(dataContainer, 1)
                );

            res.numArguments = getTFromContainer<unsigned>(dataContainer, 2);
            res.hasDotsFormals = getTFromContainer<bool>(dataContainer, 3);
            res.hasDefaultArgs = getTFromContainer<bool>(dataContainer, 4);
            res.dotsPosition = getTFromContainer<size_t>(dataContainer, 5);

            return res;
        }

        // ENTRY 1: Function Names
        static void addFNames(SEXP container, SEXP data) {
            addSEXP(container, data, 1);
        }

        static SEXP getFNames(SEXP container) {
            return VECTOR_ELT(container, 1);
        }

        // ENTRY 2: Function Sources
        static void addFSrc(SEXP container, SEXP data) {
            addSEXP(container, data, 2);
        }

        static SEXP getFSrc(SEXP container) {
            return VECTOR_ELT(container, 2);
        }

        // ENTRY 3: Arglist Order
        static void addFArg(SEXP container, SEXP data) {
            addSEXP(container, data, 3);
        }

        static SEXP getFArg(SEXP container) {
            return VECTOR_ELT(container, 3);
        }

        // ENTRY 4: Children Data
        static void addFChildren(SEXP container, SEXP data) {
            addSEXP(container, data, 4);
        }

        static SEXP getFChildren(SEXP container) {
            return VECTOR_ELT(container, 4);
        }

        // ENTRY 5: Constant Pool
        static void addCpool(SEXP container, SEXP data) {
            addSEXP(container, data, 5);
        }

        static SEXP getCpool(SEXP container) {
            return VECTOR_ELT(container, 5);
        }

        // ENTRY 6: Source Pool
        static void addSpool(SEXP container, SEXP data) {
            addSEXP(container, data, 6);
        }

        static SEXP getSpool(SEXP container) {
            return VECTOR_ELT(container, 6);
        }

        // ENTRY 7: Source Pool
        static void addEpoch(SEXP container, SEXP data) {
            addSEXP(container, data, 7);
        }

        static SEXP getEpoch(SEXP container) {
            return VECTOR_ELT(container, 7);
        }

        static void print(SEXP container, int space) {
            printSpace(space);
            std::cout << "== serializedPool ==" << std::endl;
            space += 2;

            printSpace(space);
            rir::FunctionSignature fs = getFS(container);
            std::cout << "ENTRY(0)[Function Signature]: " << (int)fs.envCreation << ", " << (int)fs.optimization << ", " <<  fs.numArguments << ", " << fs.hasDotsFormals << ", " << fs.hasDefaultArgs << ", " << fs.dotsPosition << std::endl;
            printSpace(space);
            std::cout << "ENTRY(1)[Function names]: [ ";
            auto fNames = getFNames(container);
            for (int i = 0; i < Rf_length(fNames); i++) {
                auto c = VECTOR_ELT(fNames, i);
                std::cout << CHAR(STRING_ELT(c, 0)) << " ";
            }
            std::cout << "]" << std::endl;

            printSpace(space);
            std::cout << "ENTRY(2)[Function Src]: [ ";
            auto fSrc = getFSrc(container);
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
            std::cout << "ENTRY(3)[Function Arglist Order]: [ ";
            auto fArg = getFArg(container);
            for (int i = 0; i < Rf_length(fArg); i++) {
                auto c = VECTOR_ELT(fArg, i);
                std::cout << TYPEOF(c) << " ";
            }
            std::cout << "]" << std::endl;

            printSpace(space);
            #if PRINT_EXTENDED_CHILDREN == 1
            std::cout << "ENTRY(4)[children Data]" << std::endl;
            #else
            std::cout << "ENTRY(4)[children Data]: ";
            #endif
            auto fChildren = getFChildren(container);
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

            printSpace(space);
            std::cout << "ENTRY(5)[Constant Pool Entries]: [ ";
            auto cpool = getCpool(container);
            for (int i = 0; i < Rf_length(cpool); i++) {
                auto c = VECTOR_ELT(cpool, i);
                std::cout << "TYPE(" << TYPEOF(c) << ") ";
                // if (TYPEOF(c) == SYMSXP) {
                //     std::cout << CHAR(PRINTNAME(c)) << std::endl;
                // } else if (TYPEOF(c) == RAWSXP) {
                //     if (Rf_length(c) == 1045) {
                //         DeoptMetadata* m = (DeoptMetadata *)DATAPTR(c);
                //         // m->print(std::cout);
                //         for (size_t i = 0; i < m->numFrames; i++) {
                //             if (m->frames[i].code == 0) {
                //                 auto hast = m->frames[i].hast;
                //                 int index = m->frames[i].index;

                //                 std::cout << "hast:" << hast << std::endl;
                //                 std::cout << "index:" << index << std::endl;

                //                 // m->frames[i].pc = (Opcode*)((uintptr_t)code + m->frames[i].offset);
                //             }
                //         }
                //     }
                //     std::cout << "RAW[" << Rf_length(c) << "]" << std::endl;
                // } else if (TYPEOF(c) == INTSXP) {
                //     std::cout << "INTSXP(" << Rf_length(c) << ")" << std::endl;
                //     for (int k = 0; k < Rf_length(c); k++) {
                //         std::cout << "   " << INTEGER(c)[k] << ")" << std::endl;
                //     }
                // } else if (TYPEOF(c) == LANGSXP) {
                //     std::cout << "LANGSXP: " << c << std::endl;
                // }
                // std::cout << " ";
            }
            std::cout << "]" << std::endl;

            printSpace(space);
            std::cout << "ENTRY(6)[Source Pool Entries]: [ ";
            auto spool = getSpool(container);
            for (int i = 0; i < Rf_length(spool); i++) {
                auto c = VECTOR_ELT(spool, i);
                std::cout << TYPEOF(c) << " ";
            }
            std::cout << "]" << std::endl;
            printSpace(space);
            std::cout << "ENTRY(7)[Epoch]: " << *((size_t *) DATAPTR(getEpoch(container))) << std::endl;
        }
    };

    class contextData {
        // 0 (unsigned long) unsigned long
        // 1 (SEXP) reqMapForCompilation
        // 2 (SEXP) Speculative Context
        // 3 (unsigned int) creationIndex

        public:
            // Misc functions
            static unsigned getStorageSize() {
                return 4;
            }

            static void addSEXP(SEXP container, SEXP data, const int & index) {
                SET_VECTOR_ELT(container, index, data);
            }

            static SEXP getSEXP(SEXP container, const int & index) {
                return VECTOR_ELT(container, index);
            }

            static void printSpace(int size) {
                assert(size >= 0);
                for (int i = 0; i < size; i++) {
                    std::cout << " ";
                }
            }

            // ENTRY 0: Con
            static void addContext(SEXP container, const unsigned long & data) {
                rir::Protect protecc;
                SEXP store;
                protecc(store = Rf_allocVector(RAWSXP, sizeof(unsigned long)));
                unsigned long * tmp = (unsigned long *) DATAPTR(store);
                *tmp = data;
                SET_VECTOR_ELT(container, 0, store);
            }

            static unsigned long getContext(SEXP container) {
                SEXP dataContainer = VECTOR_ELT(container, 0);
                unsigned long* res = (unsigned long *) DATAPTR(dataContainer);
                return *res;
            }


            // ENTRY 1: reqMap
            static void addReqMapForCompilation(SEXP container, SEXP data) {
                addSEXP(container, data, 1);
            }

            static SEXP getReqMapAsVector(SEXP container) {
                return getSEXP(container, 1);
            }

            static void removeEleFromReqMap(SEXP container, SEXP ele) {
                auto rData = getReqMapAsVector(container);
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
                    rir::Protect protecc;
                    SEXP newVec;
                    protecc(newVec = Rf_allocVector(VECSXP, Rf_length(rData) - 1));

                    int newVecIdx = 0;
                    for (int i = 0; i < Rf_length(rData); i++) {
                        SEXP e = VECTOR_ELT(rData, i);
                        if (i != containsElement) {
                            SET_VECTOR_ELT(newVec, newVecIdx++, e);
                        }
                    }

                    addReqMapForCompilation(container, newVec);

                }
            }

            // ENTRY 2: SpeculativeContext
            static void addSpeculativeContext(SEXP container, SEXP data) {
                assert(TYPEOF(data) == VECSXP);
                addSEXP(container, data, 2);
            }

            static SEXP getSpeculativeContext(SEXP container) {
                return getSEXP(container, 2);
            }

            // ENTRY 3: creationIndex
            static void addCI(SEXP container) {
                rir::Protect protecc;
                static unsigned int CI = 0;
                SEXP store;
                protecc(store = Rf_allocVector(RAWSXP, sizeof(unsigned int)));
                unsigned int * tmp = (unsigned int *) DATAPTR(store);
                *tmp = CI;
                CI++;
                SET_VECTOR_ELT(container, 3, store);
            }

            static unsigned int getCI(SEXP container) {
                SEXP dataContainer = VECTOR_ELT(container, 3);
                unsigned int* res = (unsigned int *) DATAPTR(dataContainer);
                return *res;
            }


            static void print(SEXP container, unsigned int space) {
                printSpace(space);
                std::cout << "== contextData ==" << std::endl;
                space += 2;

                printSpace(space);
                std::cout << "ENTRY(0)[Context]: " << rir::Context(getContext(container)) << std::endl;
                space += 2;

                auto rData = getReqMapAsVector(container);
                printSpace(space);
                std::cout << "ENTRY(1)[reqMapForCompilation]: <";
                for (int i = 0; i < Rf_length(rData); i++) {
                    SEXP ele = VECTOR_ELT(rData, i);
                    std::cout << CHAR(PRINTNAME(ele)) << " ";
                }
                std::cout << ">" << std::endl;

                SEXP scContainer = getSpeculativeContext(container);
                printSpace(space);
                std::cout << "ENTRY(2)[Speculative Context](" << Rf_length(scContainer) << " Entries)" << std::endl;
                for (int i = 0; i < Rf_length(scContainer); i++) {
                    SEXP curr = VECTOR_ELT(scContainer, i);
                    SEXP currHast = VECTOR_ELT(curr, 0);
                    SEXP currCon = VECTOR_ELT(curr, 1);
                    printSpace(space + 2);
                    std::cout << "Hast: " << CHAR(PRINTNAME(currHast)) << " (" << Rf_length(currCon) << " elements)" << std::endl;
                    printSpace(space + 4);
                    // std::cout << "conStore: " << currCon << " ";
                    for (int j = 0; j < Rf_length(currCon); j++) {
                        std::cout << "(" << j << ")";
                        speculativeContextElement::print(VECTOR_ELT(currCon, j));
                        std::cout << " ";
                    }
                    std::cout << std::endl;
                }

                printSpace(space);
                std::cout << "ENTRY(3)[Creation Index]: " << getCI(container) << std::endl;
            }
    };

    class serializerData {
        // 0 (SEXP) Hast
        // 1 (SEXP) Name
        // 2 (SEXP) offsetMap

        public:
            // Misc functions
            static unsigned getStorageSize() {
                return 3;
            }

            static void addSEXP(SEXP container, SEXP data, const int & index) {
                SET_VECTOR_ELT(container, index, data);
            }

            static SEXP getSEXP(SEXP container, const int & index) {
                return VECTOR_ELT(container, index);
            }

            static void printSpace(int size) {
                assert(size >= 0);
                for (int i = 0; i < size; i++) {
                    std::cout << " ";
                }
            }

            // ENTRY 0: Hast
            static void addHast(SEXP container, SEXP data) {
                addSEXP(container, data, 0);
            }

            static SEXP getHast(SEXP container) {
                return getSEXP(container, 0);
            }

            // ENTRY 1: Name
            static void addName(SEXP container, SEXP data) {
                addSEXP(container, data, 1);
            }

            static SEXP getName(SEXP container) {
                return getSEXP(container, 1);
            }

            static SEXP ensureAndGetOffsetMap(SEXP container) {
                SEXP currOffsetMap = getBitcodeMap(container);
                if (TYPEOF(currOffsetMap) != ENVSXP) {
                    rir::Protect protecc;
                    SEXP tmp;
                    protecc(tmp = R_NewEnv(R_EmptyEnv,0,0));
                    addSEXP(container, tmp, 2);
                }

                SEXP res = getBitcodeMap(container);
                assert(TYPEOF(res) == ENVSXP);
                return res;
            }

            static SEXP ensureAndGetOffsetEnv(SEXP container, SEXP offsetSym) {
                ensureAndGetOffsetMap(container);
                REnvHandler oMap(getBitcodeMap(container));
                if (oMap.get(offsetSym) != nullptr && TYPEOF(oMap.get(offsetSym)) == ENVSXP) {
                    return oMap.get(offsetSym);
                } else {
                    rir::Protect protecc;
                    SEXP tmp;
                    protecc(tmp = R_NewEnv(R_EmptyEnv,0,0));
                    oMap.set(offsetSym, tmp);
                }
                SEXP res = oMap.get(offsetSym);
                assert(TYPEOF(res) == ENVSXP);
                return res;
            }

            // ENTRY 2: Add Context Data
            static SEXP addBitcodeData(SEXP container, SEXP offsetSym, SEXP context, SEXP cData) {
                using namespace std::chrono;
                SEXP offsetEnvContainer = ensureAndGetOffsetEnv(container, offsetSym);

                REnvHandler currMap(offsetEnvContainer);
                if (std::stoi(CHAR(PRINTNAME(offsetSym))) == 0) {
                    // Get rid of self dependencies, may happen in case of recursive functions
                    std::stringstream ss;
                    ss << CHAR(PRINTNAME(getHast(container))) << "_" << CHAR(PRINTNAME(context));
                    contextData::removeEleFromReqMap(cData, Rf_install(ss.str().c_str()));
                }

                contextData::addCI(cData);

                SEXP conKey = Rf_install(std::to_string(system_clock::now().time_since_epoch().count()).c_str());
                currMap.set(conKey, cData);

                return conKey;
            }

            // static void addBitcodeData(SEXP container, int offset, std::string context, SEXP cData) {
            //     addBitcodeData(container, Rf_install(std::to_string(offset).c_str()), Rf_install(context.c_str()), cData);
            // }

            static SEXP getBitcodeMap(SEXP container) {
                return getSEXP(container, 2);
            }

            static void copy(SEXP from, SEXP to) {
                addHast(to, getHast(from));
                addName(to, getName(from));

                iterate(from, [&](SEXP offsetSym, SEXP conSym, SEXP cData, bool isMask) {
                    addBitcodeData(to, offsetSym, conSym, cData);
                });
            }

            static void iterate(SEXP container, const std::function< void(SEXP, SEXP, SEXP, bool) >& callback) {
                static SEXP maskSym = Rf_install("mask");

                SEXP offsetMapContainer = ensureAndGetOffsetMap(container);

                REnvHandler offsetMap(offsetMapContainer);
                offsetMap.iterate([&](SEXP offsetSym, SEXP conDataContainer) {
                    if (TYPEOF(conDataContainer) != ENVSXP) {
                        Rf_error("Serializer data error, offset symbol bound to non ENVSXP!");
                        return;
                    }
                    REnvHandler conMap(conDataContainer);
                    conMap.iterate([&](SEXP conSym, SEXP conData){
                        if (conSym == maskSym) {
                            callback(offsetSym, conSym, conData, true);
                        } else {
                            callback(offsetSym, conSym, conData, false);
                        }
                    });
                });
            }

            static void print(SEXP container, int space) {
                printSpace(space);
                std::cout << "== serializerData ==" << std::endl;
                space += 2;

                printSpace(space);
                std::cout << "ENTRY(0)[Hast]: " << CHAR(PRINTNAME(getHast(container))) << std::endl;

                printSpace(space);
                std::cout << "ENTRY(1)[Function Name]: " << CHAR(PRINTNAME(getName(container))) << std::endl;

                space += 2;

                iterate(container, [&](SEXP offsetSym, SEXP conSym, SEXP cData, bool isMask) {
                    if (!isMask) {
                        printSpace(space);
                        std::cout << "At Offset: " << CHAR(PRINTNAME(offsetSym)) << ", Epoch: " << CHAR(PRINTNAME(conSym)) << std::endl;
                        contextData::print(cData, space+2);
                    } else {
                        printSpace(space);
                        std::cout << "Mask At Offset: " << CHAR(PRINTNAME(offsetSym)) << std::endl;
                    }
                });

            }
    };
};;
