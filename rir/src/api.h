#ifndef API_H_
#define API_H_

#include "R/r.h"
#include "compiler/log/debug.h"
#include "runtime/Context.h"
#include <stdint.h>
#include <set>
#include <fstream>
#include "llvm/IR/Module.h"
#include "utils/Pool.h"
#include "runtime/FunctionSignature.h"

#define REXPORT extern "C"

extern int R_ENABLE_JIT;

struct contextMeta {
    unsigned long con; // 5
    int envCreation;       // 6
    int optimization;      // 7
    unsigned numArguments; // 8
    size_t dotsPosition;   // 9
    size_t mainNameLen;    // 10
    std::string mainName;  // 11
    size_t cPoolEntriesSize; // 12
    size_t srcPoolEntriesSize; // 13
    // size_t ePoolEntriesSize;   // 14
    size_t promiseSrcPoolEntriesSize; // 15
    size_t childrenDataLength; // 16
    std::string childrenData;  // 17
    size_t srcDataLength;      // 18
    std::string srcData;       // 19
    size_t argDataLength;      // 20
    std::string argData;       // 21
    size_t arglistOrderEntriesLength; // 22
    std::vector<std::vector<std::vector<size_t>>> argOrderingData; // 23, 24, 25
    size_t reqMapSize; // 26
    std::vector<size_t> reqMapForCompilation; // 27

    void print(std::ostream & out) {
        out << "   Entry 5(context):" << con << std::endl;
        out << "   Entry 6(envCreation): " << envCreation << std::endl;
        out << "   Entry 7(optimization): " << optimization << std::endl;
        out << "   Entry 8(numArguments): " << numArguments << std::endl;
        out << "   Entry 9(dotsPosition): " << dotsPosition << std::endl;
        out << "   Entry 10(mainNameLen): " << mainNameLen << std::endl;
        out << "   Entry 11(name): " << mainName.c_str() << std::endl;
        out << "   Entry 12(cPoolEntriesSize): " << cPoolEntriesSize << std::endl;
        out << "   Entry 13(srcPoolEntriesSize): " << srcPoolEntriesSize << std::endl;
        // out << "   Entry 14(ePoolEntriesSize): " << ePoolEntriesSize << std::endl;
        out << "   Entry 15(promiseSrcPoolEntriesSize): " << promiseSrcPoolEntriesSize << std::endl;
        out << "   Entry 16(childrenDataLength): " << childrenDataLength << std::endl;
        out << "   Entry 17(childrenData): " << childrenData << std::endl;
        out << "   Entry 18(srcDataLength): " << srcDataLength << std::endl;
        out << "   Entry 19(srcData): " << srcData << std::endl;
        out << "   Entry 20(argDataLength): " << argDataLength << std::endl;
        out << "   Entry 21(argData): " << argData << std::endl;
        out << "   Entry 22(arglistOrderEntriesLength): " << arglistOrderEntriesLength << std::endl;
        for (auto & ele : argOrderingData) {
            std::cout << "      Entry 23(outerEntriesSize): " << ele.size() << std::endl;
            for (auto & outer : ele) {
                std::cout << "         Entry 24(innerEntriesSize): " << outer.size() << std::endl;
                for (auto & ele : outer) {
                    std::cout << "            Entry 25(currEntry): " << ele << std::endl;
                }
            }
        }
        std::cout << "   Entry 26(reqMapSize): " << reqMapSize << std::endl;
        for (auto & ele : reqMapForCompilation) {
            std::cout << "      Entry 27(currEntry): " << ele << std::endl;
        }
    }

    void writeContextMeta(std::ofstream & metaDataFile) {
        // Entry 5 (unsigned long): context
        metaDataFile.write(reinterpret_cast<char *>(&con), sizeof(unsigned long));

        // Entry 6 (int): envCreation
        metaDataFile.write(reinterpret_cast<char *>(&envCreation), sizeof(int));

        // Entry 7 (int): optimization
        metaDataFile.write(reinterpret_cast<char *>(&optimization), sizeof(int));

        // Entry 8 (unsigned): numArguments
        metaDataFile.write(reinterpret_cast<char *>(&numArguments), sizeof(unsigned));

        // Entry 9 (size_t): dotsPosition
        metaDataFile.write(reinterpret_cast<char *>(&dotsPosition), sizeof(size_t));

        // Entry 10 (size_t): mainNameLength
        metaDataFile.write(reinterpret_cast<char *>(&mainNameLen), sizeof(size_t));

        // Entry 11 (bytes...): name
        metaDataFile.write(mainName.c_str(), sizeof(char) * mainNameLen);

        // Entry 12 (size_t): cPoolEntriesSize
        metaDataFile.write(reinterpret_cast<char *>(&cPoolEntriesSize), sizeof(size_t));

        // Entry 13 (size_t): srcPoolEntriesSize
        metaDataFile.write(reinterpret_cast<char *>(&srcPoolEntriesSize), sizeof(size_t));

        // // Entry 14 (size_t): ePoolEntriesSize
        // metaDataFile.write(reinterpret_cast<char *>(&ePoolEntriesSize), sizeof(size_t));

        // Entry 15 (size_t): promiseSrcPoolEntriesSize - These need to be the src entries when we deserialie
        metaDataFile.write(reinterpret_cast<char *>(&promiseSrcPoolEntriesSize), sizeof(size_t));

        // Entry 16 (size_t): childrenDataLength
        metaDataFile.write(reinterpret_cast<char *>(&childrenDataLength), sizeof(size_t));

        // Entry 17 (bytes...): childrenData
        metaDataFile.write(childrenData.c_str(), sizeof(char) * childrenDataLength);

        // Entry 18 (size_t): srcDataLength
        metaDataFile.write(reinterpret_cast<char *>(&srcDataLength), sizeof(size_t));

        // Entry 19 (bytes...): srcData
        metaDataFile.write(srcData.c_str(), sizeof(char) * srcDataLength);

        // Entry 20 (size_t): argDataLength
        metaDataFile.write(reinterpret_cast<char *>(&argDataLength), sizeof(size_t));

        // Entry 21 (bytes...): argData
        metaDataFile.write(argData.c_str(), sizeof(char) * argDataLength);

        // Entry 22 (size_t): arglistOrderEntriesLength
        metaDataFile.write(reinterpret_cast<char *>(&arglistOrderEntriesLength), sizeof(size_t));

        for (auto & ele : argOrderingData) {
            // Entry 23 (size_t): outerEntriesSize
            size_t outerEntriesSize = ele.size();
            metaDataFile.write(reinterpret_cast<char *>(&outerEntriesSize), sizeof(size_t));

            for (auto & outer : ele) {
                // Entry 24 (size_t): innerEntriesSize
                size_t innerEntriesSize = outer.size();
                metaDataFile.write(reinterpret_cast<char *>(&innerEntriesSize), sizeof(size_t));

                for (auto & ele : outer) {
                    // Entry 25 (size_t): currEntry
                    size_t currEntry = ele;
                    metaDataFile.write(reinterpret_cast<char *>(&currEntry), sizeof(size_t));

                }
            }
        }

        // Entry 26 (size_t): reqMapSize
        size_t reqMapSize = reqMapForCompilation.size();
        metaDataFile.write(reinterpret_cast<char *>(&reqMapSize), sizeof(size_t));

        for (auto & ele : reqMapForCompilation) {
            // Entry 27 (size_t): currEntry
            size_t currEntry = ele;
            metaDataFile.write(reinterpret_cast<char *>(&currEntry), sizeof(size_t));
        }
    }

    static contextMeta readContextMeta(std::ifstream & metaDataFile) {
        // READ 5: CONTEXTNO
        unsigned long con = 0;
        metaDataFile.read(reinterpret_cast<char *>(&con),sizeof(unsigned long));

        // READ 6: ENVCREATION
        int envCreation = 0;
        metaDataFile.read(reinterpret_cast<char *>(&envCreation),sizeof(int));

        // READ 7: OPTIMIZATION
        int optimization = 0;
        metaDataFile.read(reinterpret_cast<char *>(&optimization),sizeof(int));

        // READ 8: NUM ARGS
        unsigned int numArguments = 0;
        metaDataFile.read(reinterpret_cast<char *>(&numArguments),sizeof(unsigned int));

        // READ 9: DOTS POS
        size_t dotsPosition = 0;
        metaDataFile.read(reinterpret_cast<char *>(&dotsPosition),sizeof(size_t));

        // READ 10: MAIN FUNCTION LENGTH
        size_t mainNameLen = 0;
        metaDataFile.read(reinterpret_cast<char *>(&mainNameLen),sizeof(size_t));

        // READ 11: MAIN FUNCTION NAME
        char * mainName;
        mainName = new char [mainNameLen + 1];
        metaDataFile.read(mainName,mainNameLen);
        mainName[mainNameLen] = '\0';

        // READ 12: ConstantPoolEntries
        size_t cPoolEntriesSize = 0;
        metaDataFile.read(reinterpret_cast<char *>(&cPoolEntriesSize),sizeof(size_t));

        // READ 13: SourcePoolEntries
        size_t srcPoolEntriesSize = 0;
        metaDataFile.read(reinterpret_cast<char *>(&srcPoolEntriesSize),sizeof(size_t));

        // READ 14: ExtraPoolEntries
        // size_t ePoolEntriesSize = 0;
        // metaDataFile.read(reinterpret_cast<char *>(&ePoolEntriesSize),sizeof(size_t));

        // READ 15: ExtraPoolEntries
        size_t promiseSrcPoolEntriesSize = 0;
        metaDataFile.read(reinterpret_cast<char *>(&promiseSrcPoolEntriesSize),sizeof(size_t));

        // Entry 16 (size_t): childrenDataLength
        size_t childrenDataLength = 0;
        metaDataFile.read(reinterpret_cast<char *>(&childrenDataLength),sizeof(size_t));

        // Entry 17 (bytes...): name
        char * childrenData;
        childrenData = new char [childrenDataLength + 1];
        metaDataFile.read(childrenData,childrenDataLength);
        childrenData[childrenDataLength] = '\0';

        // Entry 18 (size_t): srcDataLength
        size_t srcDataLength = 0;
        metaDataFile.read(reinterpret_cast<char *>(&srcDataLength),sizeof(size_t));

        // Entry 19 (bytes...): name
        char * srcData;
        srcData = new char [srcDataLength + 1];
        metaDataFile.read(srcData,srcDataLength);
        srcData[srcDataLength] = '\0';

        // Entry 20 (size_t): argDataLength
        size_t argDataLength = 0;
        metaDataFile.read(reinterpret_cast<char *>(&argDataLength),sizeof(size_t));

        // Entry 21 (bytes...): argData
        char * argData;
        argData = new char [argDataLength + 1];
        metaDataFile.read(argData,argDataLength);
        argData[argDataLength] = '\0';

        // Entry 22 (size_t): arglistOrderEntriesLength
        size_t arglistOrderEntriesLength = 0;
        metaDataFile.read(reinterpret_cast<char *>(&arglistOrderEntriesLength),sizeof(size_t));

        std::vector<std::vector<std::vector<size_t>>> argOrderingData;

        for (size_t i = 0; i < arglistOrderEntriesLength; i++) {
            // Entry 23 (size_t): outerEntriesSize
            size_t outerEntriesSize = 0;
            metaDataFile.read(reinterpret_cast<char *>(&outerEntriesSize),sizeof(size_t));

            std::vector<std::vector<size_t>> outerData;
            for (size_t j = 0; j < outerEntriesSize; j++) {
                // Entry 24 (size_t): innerEntriesSize
                size_t innerEntriesSize = 0;
                metaDataFile.read(reinterpret_cast<char *>(&innerEntriesSize),sizeof(size_t));

                std::vector<size_t> innerData;
                for (size_t k = 0; k < innerEntriesSize; k++) {
                    size_t currEntry = 0;
                    // Entry 25 (size_t): currEntry
                    metaDataFile.read(reinterpret_cast<char *>(&currEntry),sizeof(size_t));

                    innerData.push_back(currEntry);
                }

                outerData.push_back(innerData);
            }

            argOrderingData.push_back(outerData);
        }

        // Entry 26 (size_t): reqMapSize
        size_t reqMapSize = 0;
        metaDataFile.read(reinterpret_cast<char *>(&reqMapSize),sizeof(size_t));

        std::vector<size_t> reqMapForCompilation;

        for (size_t i = 0; i < reqMapSize; i++) {
            size_t currEntry = 0;
            // Entry 27 (size_t): currEntry
            metaDataFile.read(reinterpret_cast<char *>(&currEntry),sizeof(size_t));
            reqMapForCompilation.push_back(currEntry);
        }

        contextMeta res;
        res.con = con;
        res.envCreation = envCreation;
        res.optimization = optimization;
        res.numArguments = numArguments;
        res.dotsPosition = dotsPosition;
        res.mainNameLen = mainNameLen;
        res.mainName = std::string(mainName);
        delete[] mainName;
        res.cPoolEntriesSize = cPoolEntriesSize;
        res.srcPoolEntriesSize = srcPoolEntriesSize;
        // res.ePoolEntriesSize = ePoolEntriesSize;
        res.promiseSrcPoolEntriesSize = promiseSrcPoolEntriesSize;
        res.childrenDataLength = childrenDataLength;
        res.childrenData = std::string(childrenData);
        delete[] childrenData;
        res.srcDataLength = srcDataLength;
        res.srcData = std::string(srcData);
        res.argDataLength = argDataLength;
        res.argData = std::string(argData);
        delete[] argData;
        res.arglistOrderEntriesLength = arglistOrderEntriesLength;
        res.argOrderingData = argOrderingData;
        res.reqMapSize = reqMapSize;
        res.reqMapForCompilation = reqMapForCompilation;

        return res;
    }
};

struct hastMeta {
    int nameLen;
    std::string name;
    size_t hast;
    int numVersions;

    hastMeta() {}

    hastMeta(int nameLen, std::string name, size_t hast, int numVersions)
        : nameLen(nameLen), name(name), hast(hast), numVersions(numVersions) {
    }

    void print(std::ostream & out) {
        out << "Entry 1(nameLen)    : " << nameLen << std::endl;
        out << "Entry 2(name)       : " << name << std::endl;
        out << "Entry 3(hast)       : " << hast << std::endl;
        out << "Entry 4(numVersions): " << numVersions << std::endl;
    }

    void writeHastMeta(std::ofstream & metaDataFile) {
        // Entry 1 (int): length of string containing the name
        metaDataFile.write(reinterpret_cast<char *>(&nameLen), sizeof(int));

        // Entry 2 (bytes...): bytes containing the name
        metaDataFile.write(name.c_str(), sizeof(char) * nameLen);

        // Entry 3 (size_t): hast of the function
        metaDataFile.write(reinterpret_cast<char *>(&hast) , sizeof(size_t));

        // Entry 4 (int): number of contexts serialized
        metaDataFile.write(reinterpret_cast<char *>(&numVersions), sizeof(int));
    }

    static hastMeta readHastMeta(std::ifstream & metaDataFile) {
        // READ 1: (int) nameLength
        int nameLen = 0;
        metaDataFile.read(reinterpret_cast<char *>(&nameLen),sizeof(int));

        // READ 2: (bytes...) functionName
        char * functionName = new char [nameLen + 1];
        metaDataFile.read(functionName,sizeof(char) * nameLen);
        functionName[nameLen] = '\0';
        std::string name(functionName);
        delete[] functionName;

        // READ 3: (size_t) hast
        size_t hast = 0;
        metaDataFile.read(reinterpret_cast<char *>(&hast),sizeof(size_t));

        // READ 4: (int) versions
        int versions = 0;
        metaDataFile.read(reinterpret_cast<char *>(&versions),sizeof(int));

        hastMeta result = {
            nameLen,
            name,
            hast,
            versions
        };
        return result;
    }

};

struct hastAndIndex {
    size_t hast;
    int index;
};

REXPORT SEXP rirInvocationCount(SEXP what);
REXPORT SEXP pirCompileWrapper(SEXP closure, SEXP name, SEXP debugFlags,
                               SEXP debugStyle);
REXPORT SEXP rirCompile(SEXP what, SEXP env);
REXPORT SEXP pirTests();
REXPORT SEXP pirCheck(SEXP f, SEXP check, SEXP env);
REXPORT SEXP pirSetDebugFlags(SEXP debugFlags);
SEXP pirCompile(SEXP closure, const rir::Context& assumptions,
                const std::string& name, const rir::pir::DebugOptions& debug);

extern SEXP rirOptDefaultOpts(SEXP closure, const rir::Context&, SEXP name);
extern SEXP rirOptDefaultOptsDryrun(SEXP closure, const rir::Context&,
                                    SEXP name);

void hash_ast(SEXP ast, size_t & hast);
void printAST(int space, SEXP ast);
void printAST(int space, int val);

hastAndIndex getHastAndIndex(unsigned src);

REXPORT SEXP startSerializer();
REXPORT SEXP stopSerializer();

REXPORT SEXP printHAST(SEXP clos);
REXPORT SEXP rirSerialize(SEXP data, SEXP file);
REXPORT SEXP rirDeserialize(SEXP file);

REXPORT SEXP rirSetUserContext(SEXP f, SEXP udc);
REXPORT SEXP rirCreateSimpleIntContext();

#endif // API_H_
