/** Enables the use of R internals for us so that we can manipulate R structures
 * in low level.
 */

#include "api.h"
#include "R/Funtab.h"
#include "R/Serialize.h"
#include "compiler/backend.h"
#include "compiler/compiler.h"
#include "compiler/log/debug.h"
#include "compiler/parameter.h"
#include "compiler/pir/closure.h"
#include "compiler/test/PirCheck.h"
#include "compiler/test/PirTests.h"
#include "interpreter/interp_incl.h"

#include "utils/Pool.h"
#include "compiler/native/types_llvm.h"

#include "patches.h"

#include "ir/BC.h"
#include "ir/Compiler.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <cassert>
#include <cstdio>
#include <list>
#include <memory>
#include <string>

using namespace rir;

extern "C" Rboolean R_Visible;

int R_ENABLE_JIT = getenv("R_ENABLE_JIT") ? atoi(getenv("R_ENABLE_JIT")) : 3;

static size_t oldMaxInput = 0;
static size_t oldInlinerMax = 0;
static bool oldPreserve = false;
static unsigned oldSerializeChaos = false;
static bool oldDeoptChaos = false;

const std::vector<std::string> BaseLibs::libBaseName = {
    "match.fun"
};

const std::vector<std::size_t> BaseLibs::libBaseHast = {
    26989 // 0
};


bool parseDebugStyle(const char* str, pir::DebugStyle& s) {
#define V(style)                                                               \
    if (strcmp(str, #style) == 0) {                                            \
        s = pir::DebugStyle::style;                                            \
        return true;                                                           \
    } else
    LIST_OF_DEBUG_STYLES(V)
#undef V
    {
        return false;
    }
}

REXPORT SEXP rirDisassemble(SEXP what, SEXP verbose) {
    if (!what || TYPEOF(what) != CLOSXP)
        Rf_error("Not a rir compiled code (Not CLOSXP)");
    DispatchTable* t = DispatchTable::check(BODY(what));

    if (!t)
        Rf_error("Not a rir compiled code (CLOSXP but not DispatchTable)");

    std::cout << "== closure " << what << " (dispatch table " << t << ", env "
              << CLOENV(what) << ") ==\n";
    for (size_t entry = 0; entry < t->size(); ++entry) {
        Function* f = t->get(entry);
        std::cout << "= version " << entry << " (" << f << ") =\n";
        f->disassemble(std::cout);
    }

    return R_NilValue;
}

static int charToInt(const char* p) {
    int result = 0;
    for (size_t i = 0; i < strlen(p); ++i) {
        result += p[i];
    }
    return result;
}


void printSpace(int & lim) {
    int i = 0;
    for(i = 0; i < lim; i++ ) {
        std::cout << " ";
    }
}

void printHeader(int & space, const char * title) {
    std::cout << " » " << title << "}" << std::endl;
    space++;
}

void printType(int & space, const char * attr, SEXP ptr) {
    printSpace(space);
    std::cout << "└■ " << attr << " {" << TYPEOF(ptr);
}

void printType(int & space, const char * attr, int val) {
    printSpace(space);
    std::cout << "└■ " << attr << " {" << val;
}

void printSPECIALSXP(int space, SEXP specialsxp) {
    printHeader(space, "SPECIALSXP");
}

void printLangSXP(int space, SEXP langsxp) {
    printHeader(space, "LANGSXP");

    auto tag = TAG(langsxp);
    auto car = CAR(langsxp);
    auto cdr = CDR(langsxp);

    printType(space, "TAG", tag);
    printAST(space, tag);

    printType(space, "CAR", car);
    printAST(space, car);

    printType(space, "CDR", cdr);
    printAST(space, cdr);
}

void printSYMSXP(int space, SEXP symsxp) {
    printHeader(space, "SYMSXP");

    auto pname = PRINTNAME(symsxp);
    auto value = SYMVALUE(symsxp);
    auto internal = INTERNAL(symsxp);

    printType(space, "PNAME", pname);
    printAST(space, pname);

    printType(space, "VALUE", value);
    if (symsxp != value) {
        // printAST(space, value);
        std::cout << "}" << std::endl;
    } else {
        std::cout << "}" << std::endl;
    }

    // std::cout << "}" << std::endl;

    printType(space, "INTERNAL", internal);
    printAST(space, internal);
}

void printCHARSXP(int space, SEXP charSXP) {
    printHeader(space, "CHARSXP");

    printSpace(space);
    std::cout << CHAR(charSXP) << std::endl;
}

void printSTRSXP(int space, SEXP strSXP) {
    printHeader(space, "STRSXP");

    printSpace(space);
    std::cout << CHAR(STRING_ELT(strSXP, 0)) << std::endl;
}

void printREALSXP(int space, SEXP realSXP) {
    printHeader(space, "REALSXP");

    printSpace(space);
    std::cout << *REAL(realSXP) << std::endl;
}

void printLISTSXP(int space, SEXP listsxp) {
    printHeader(space, "LISTSXP");

    auto tag = TAG(listsxp);
    auto car = CAR(listsxp);
    auto cdr = CDR(listsxp);

    printType(space, "TAG", tag);
    printAST(space, tag);

    printType(space, "CAR", car);
    printAST(space, car);

    printType(space, "CDR", cdr);
    printAST(space, cdr);

}

void printCLOSXP(int space, SEXP closxp) {
    printHeader(space, "CLOSXP");

    auto formals = FORMALS(closxp);
    auto body = BODY(closxp);
    auto cloenv = CLOENV(closxp);

    printType(space, "FORMALS", formals);
    printAST(space, formals);

    printType(space, "BODY", body);
    printAST(space, body);

    printType(space, "CLOENV", cloenv);
    printAST(space, cloenv);

}

void printExternalCodeEntry(int space, SEXP externalsxp) {
    printHeader(space, "EXTERNALSXP");
    if (Code::check(externalsxp)) {
        Code * code = Code::unpack(externalsxp);
        code->print(std::cout);
    }
}

void printBCODESXP(int space, SEXP bcodeSXP) {
    printHeader(space, "BCODESXP");
    printType(space, "VECTOR_ELT(CDR(BCODESXP),0)", bcodeSXP);
    printAST(space, VECTOR_ELT(CDR(bcodeSXP),0));
}

void printPROMSXP(int space, SEXP promSXP) {
    printHeader(space, "PROMSXP");

    auto seen = PRSEEN(promSXP);
    auto code = PRCODE(promSXP);
    auto env = PRENV(promSXP);
    auto value = PRVALUE(promSXP);

    printType(space, "SEEN", seen);
    printAST(space, seen);

    printType(space, "CODE", code);
    printAST(space, code);

    printType(space, "ENV", env);
    printAST(space, env);

    printType(space, "VALUE", value);
    printAST(space, value);
}

void printENVSXP(int space, SEXP envSXP) {
    printHeader(space, "ENVSXP");

    auto frame = FRAME(envSXP);
    auto encls = FRAME(envSXP);
    auto hashtab = FRAME(envSXP);


    printType(space, "FRAME", frame);
    printAST(space, frame);

    printType(space, "ENCLS", encls);
    printAST(space, encls);

    printType(space, "HASHTAB", hashtab);
    printAST(space, hashtab);


}

void printRAWSXP(int space, SEXP rawSXP) {
    printHeader(space, "ENVSXP");

    Rbyte * rawData = RAW(rawSXP);

    printSpace(space);
    std::cout << *rawData << std::endl;

}

void printAST(int space, int val) {
    std::cout << val << "}" << std::endl;
}


void printAST(int space, SEXP ast) {
    switch(TYPEOF(ast)) {
        case CLOSXP: printCLOSXP(++space, ast); break;
        case LANGSXP: printLangSXP(++space, ast); break;
        case SYMSXP: printSYMSXP(++space, ast); break;
        case LISTSXP: printLISTSXP(++space, ast); break;
        case CHARSXP: printCHARSXP(++space, ast); break;
        case STRSXP: printSTRSXP(++space, ast); break;
        case REALSXP: printREALSXP(++space, ast); break;
        case BCODESXP: printBCODESXP(++space, ast); break;
        case PROMSXP: printPROMSXP(++space, ast); break;
        case ENVSXP: printENVSXP(++space, ast); break;
        case RAWSXP: printRAWSXP(++space, ast); break;
        case SPECIALSXP: printSPECIALSXP(++space, ast); break;
        case EXTERNALSXP: printExternalCodeEntry(++space, ast); break;
        default: std::cout << "}" << std::endl; break;
    }
}


void hash_ast(SEXP ast, size_t & hast) {
    if (TYPEOF(ast) == LISTSXP || TYPEOF(ast) == LANGSXP) {
        if (TYPEOF(CAR(ast)) == SYMSXP) {
            const char * pname = CHAR(PRINTNAME(CAR(ast)));
            hast += charToInt(pname);
            return hash_ast(CDR(ast), ++hast);
        }
        hash_ast(CAR(ast), ++hast);
        hash_ast(CDR(ast), ++hast);
    }
}

REXPORT SEXP printHAST(SEXP clos) {
    if (DispatchTable::check(BODY(clos))) {
        auto vtable = DispatchTable::unpack(BODY(clos));
        std::cout << "hast: " << vtable->baseline()->body()->hast << std::endl;
        return R_TrueValue;
    }

    return R_FalseValue;
}

REXPORT SEXP rirCompile(SEXP what, SEXP env) {
    if (TYPEOF(what) == CLOSXP) {
        SEXP body = BODY(what);
        if (TYPEOF(body) == EXTERNALSXP)
            return what;

        // std::cout << "rirCompile: " << what << std::endl;

        // Change the input closure inplace
        Compiler::compileClosure(what);

        return what;
    } else {
        if (TYPEOF(what) == BCODESXP) {
            what = VECTOR_ELT(CDR(what), 0);
        }
        SEXP result = Compiler::compileExpression(what);
        return result;
    }
}

REXPORT SEXP vSerialize(SEXP fun, SEXP funName, SEXP versions) {
    if (TYPEOF(fun) == CLOSXP) {
        std::ofstream metaDataFile;

        // Get or compile bytecode
        auto compiledFun = rirCompile(fun, R_EmptyEnv);
        DispatchTable* vtable = DispatchTable::unpack(BODY(compiledFun));

        // Hast is calculated at bytecode compilation time,
        auto hast = vtable->baseline()->body()->hast;

        pir::DebugOptions opts = pir::DebugOptions::DefaultDebugOptions;
        std::string name;
        if (TYPEOF(funName) == SYMSXP)
            name = CHAR(PRINTNAME(funName));


        std::vector<unsigned long> contextList;

        if (TYPEOF(versions) == NILSXP) {
            for (size_t i = 1; i < vtable->size(); i++) {
                contextList.push_back(vtable->get(i)->context().toI());
            }
        } else if (TYPEOF(versions) == REALSXP) {
            for (int i = 0; i < Rf_length(versions); i++) {
                contextList.push_back(REAL(versions)[i]);
            }
        } else {
            Rf_error("Invalid context list");
        }

        if (contextList.size() == 0) {
            Rf_error("No contexts found");
        }


        if (strlen(name.c_str()) == 0) {
            name = "UKN";
        }

        std::stringstream ss;
        ss << name << "_" << hast << ".meta";

        metaDataFile.open(ss.str(), std::ios::binary | std::ios::out);

        // First line of the file contains generic metadata about the function
        int nameLen = strlen(name.c_str());

        // Entry 1 (int): length of string containing the name
        metaDataFile.write(reinterpret_cast<char *>(&nameLen), sizeof(int));

        // Entry 2 (bytes...): bytes containing the name
        metaDataFile.write(name.c_str(), sizeof(char) * nameLen);

        // Entry 3 (size_t): hast of the function
        metaDataFile.write(reinterpret_cast<char *>(&hast) , sizeof(size_t));

        // Entry 4 (int): number of contexts serialized
        int s = contextList.size();
        metaDataFile.write(reinterpret_cast<char *>(&s), sizeof(int));

        #if API_DEBUG_CALLBACKS == 1
        std::cout << "Entry 1(nameLen): " << nameLen << std::endl;
        std::cout << "Entry 2(name): " << name.c_str() << std::endl;
        std::cout << "Entry 3(hast): " << hast << std::endl;
        std::cout << "Entry 4(number of contexts): " << s << std::endl;
        #endif

        #if PRINT_SERIALIZER_PROGRESS == 1
        std::cout << "Serializing bitcode: " << name << "(" << hast << ")" << ", found " << contextList.size() << " version(s)" << std::endl;
        #endif
        for (size_t i = 0; i < contextList.size(); i++) {

            // Looping over all the contexts
            Context c(contextList.at(i));
            unsigned long con = c.toI();

            #if PRINT_SERIALIZER_PROGRESS == 1
            std::cout << "(" << con << ") : " << c << std::endl;
            #endif

            // This is the number of constant pool entries to be patched
            size_t cPoolEntriesSize = 0;

            // This is the number of source pool entries to be patched
            size_t srcPoolEntriesSize = 0;

            // This represents the number of extra pool entries that will be needed to be inserted
            // into the extra pool upon deserialization
            size_t ePoolEntriesSize = 0;

            // This is the number of source pool entries relating to the promises
            size_t promiseSrcPoolEntriesSize = 0;

            #if API_DEBUG_CALLBACKS == 1
            std::cout << "Entry 5 [" << i << "] :" << std::endl;
            std::cout << "Context: " << con << std::endl;
            #endif

            // Entry 5 (unsigned long): context
            metaDataFile.write(reinterpret_cast<char *>(&con), sizeof(unsigned long));

            auto signatureWriter = [&](FunctionSignature & fs, std::string mainName) {

                // Entry 6 (int): envCreation
                int envCreation = (int)fs.envCreation;
                metaDataFile.write(reinterpret_cast<char *>(&envCreation), sizeof(int));

                // Entry 7 (int): optimization
                int optimization = (int)fs.optimization;
                metaDataFile.write(reinterpret_cast<char *>(&optimization), sizeof(int));

                // Entry 8 (unsigned): numArguments
                unsigned numArguments = fs.numArguments;
                metaDataFile.write(reinterpret_cast<char *>(&numArguments), sizeof(unsigned));

                // Entry 9 (size_t): dotsPosition
                size_t dotsPosition = fs.dotsPosition;
                metaDataFile.write(reinterpret_cast<char *>(&dotsPosition), sizeof(size_t));

                // Entry 10 (size_t): mainNameLength
                size_t mainNameLen = strlen(mainName.c_str());
                metaDataFile.write(reinterpret_cast<char *>(&mainNameLen), sizeof(size_t));

                // Entry 11 (bytes...): name
                metaDataFile.write(mainName.c_str(), sizeof(char) * mainNameLen);

                // Entry 12 (size_t): cPoolEntriesSize
                metaDataFile.write(reinterpret_cast<char *>(&cPoolEntriesSize), sizeof(size_t));

                // Entry 13 (size_t): srcPoolEntriesSize
                metaDataFile.write(reinterpret_cast<char *>(&srcPoolEntriesSize), sizeof(size_t));

                // Entry 14 (size_t): ePoolEntriesSize
                metaDataFile.write(reinterpret_cast<char *>(&ePoolEntriesSize), sizeof(size_t));

                // Entry 15 (size_t): promiseSrcPoolEntriesSize - These need to be the src entries when we deserialie
                metaDataFile.write(reinterpret_cast<char *>(&promiseSrcPoolEntriesSize), sizeof(size_t));


                #if API_DEBUG_CALLBACKS == 1
                std::cout << "envCreation: " << envCreation << std::endl;
                std::cout << "optimization: " << optimization << std::endl;
                std::cout << "numArguments: " << numArguments << std::endl;
                std::cout << "dotsPosition: " << dotsPosition << std::endl;
                std::cout << "mainNameLen: " << mainNameLen << std::endl;
                std::cout << "name: " << mainName.c_str() << std::endl;
                std::cout << "cPoolEntriesSize: " << cPoolEntriesSize << std::endl;
                std::cout << "srcPoolEntriesSize: " << srcPoolEntriesSize << std::endl;
                std::cout << "ePoolEntriesSize: " << ePoolEntriesSize << std::endl;
                std::cout << "promiseSrcPoolEntriesSize: " << promiseSrcPoolEntriesSize << std::endl;
                #endif

            };

            auto serializeCallback = [&](llvm::Module* m, rir::Code * code, std::vector<unsigned> & srcIndices) {
                if (m) {
                    // We clone the module because we dont want to update constant pool references in the original module
                    auto module = llvm::CloneModule(*m);
                    std::vector<int64_t> cpEntries;
                    std::vector<int64_t> spEntries;
                    std::vector<SEXP> ePoolEntries;

                    #if API_PRINT_MODULE_BEFORE_POOL_PATCHES == 1
                    llvm::raw_os_ostream dbg_stream(std::cout);
                    dbg_stream << *m;
                    dbg_stream << "\n";
                    #endif

                    int patchValue = 0;

                    // Iterating over all globals
                    for (auto & global : module->getGlobalList()) {
                        auto pre = global.getName().str().substr(0,6) == "copool";

                        auto srp = global.getName().str().substr(0,6) == "srpool";

                        auto epe = global.getName().str().substr(0, 4) == "epe_"; // extra pool entry for deopts

                        // All src pool references have a srpool prefix
                        if (srp) {
                            auto con = global.getInitializer();
                            if (auto * v = llvm::dyn_cast<llvm::ConstantInt>(con)) {
                                // Simple constant ints
                                auto val = v->getSExtValue();
                                spEntries.push_back(val);

                                // Offset relative to the serialized source pool
                                llvm::Constant* replacementValue = llvm::ConstantInt::get(rir::pir::PirJitLLVM::getContext(), llvm::APInt(32, srcPoolEntriesSize++));

                                global.setInitializer(replacementValue);
                            }
                        }

                        // All constant pool references have a copool prefix
                        if (pre) {
                            llvm::raw_os_ostream ost(std::cout);
                            auto con = global.getInitializer();
                            if (auto * v = llvm::dyn_cast<llvm::ConstantDataArray>(con)) {

                                // Constant data array
                                std::vector<llvm::Constant*> patchedIndices;

                                auto arrSize = v->getNumElements();

                                for (unsigned int i = 0; i < arrSize; i++) {
                                    auto val = v->getElementAsAPInt(i).getSExtValue();
                                    cpEntries.push_back(val);

                                    // Offset relative to the serialized pool
                                    llvm::Constant* replacementValue = llvm::ConstantInt::get(rir::pir::PirJitLLVM::getContext(), llvm::APInt(32, patchValue++));
                                    patchedIndices.push_back(replacementValue);

                                }

                                auto ty = llvm::ArrayType::get(rir::pir::t::Int, patchedIndices.size());
                                auto newInit = llvm::ConstantArray::get(ty, patchedIndices);

                                global.setInitializer(newInit);
                            } else if (auto * v = llvm::dyn_cast<llvm::ConstantInt>(con)) {
                                // Simple constant ints
                                auto val = v->getSExtValue();
                                cpEntries.push_back(val);

                                // Offset relative to the serialized pool
                                llvm::Constant* replacementValue = llvm::ConstantInt::get(rir::pir::PirJitLLVM::getContext(), llvm::APInt(32, patchValue++));

                                global.setInitializer(replacementValue);
                            }
                            // else  if (auto * v = llvm::dyn_cast<llvm::ConstantAggregateZero>(con)) {
                            //     // Zero Initialized array
                            //     #if API_PRINT_MISC_MSG == 1
                            //     std::cout << "Constant Aggregate zero: " <<  global.getName().str() << std::endl;
                            //     #endif
                            // } else  if (auto * v = llvm::dyn_cast<llvm::ConstantStruct>(con)) {
                            //     // the symbol used in this should be added to HAST_REQUIRED list
                            //     #if API_PRINT_MISC_MSG == 1
                            //     std::cout << "Constant Struct(Deopt Reason): " <<  global.getName().str() << std::endl;
                            //     #endif
                            // } else {
                            //     llvm::raw_os_ostream ooo(std::cout);
                            //     ooo << *module;
                            //     std::cout << "Unknown constant pool entry type: " << global.getName().str() << std::endl;
                            //     Rf_error("Unknown constant pool entry");
                            // }
                        }

                        if (epe) {
                            auto firstDel = global.getName().str().find('_');
                            auto secondDel = global.getName().str().find('_', firstDel + 1);
                            auto thirdDel = global.getName().str().find('_', secondDel + 1);
                            auto extraPoolOffset = std::stoi(global.getName().str().substr(secondDel + 1, thirdDel - secondDel - 1));

                            SEXP entry = code->getExtraPoolEntry(extraPoolOffset);
                            ePoolEntries.push_back(entry);

                            ePoolEntriesSize++;

                        }
                    }

                    // Src Indices for the promises
                    promiseSrcPoolEntriesSize = srcIndices.size();

                    // Restore the original bodies of the closures
                    std::unordered_map<SEXP, SEXP> restoreMap;

                    // Creating a vector containing all constant pool references
                    auto serializationObjects = Rf_allocVector(VECSXP, cpEntries.size() + spEntries.size() + ePoolEntries.size() + promiseSrcPoolEntriesSize);

                    cPoolEntriesSize   = cpEntries.size();
                    srcPoolEntriesSize = spEntries.size();
                    ePoolEntriesSize   = ePoolEntries.size();

                    int i = 0;

                    #if API_DEBUG_CALLBACKS == 1
                    std::cout << "Constant Pool Entries: [ ";
                    #endif

                    for (auto & ele : cpEntries) {
                        SEXP obj = Pool::get(ele);
                        // Dont serialize external code, we serialize the original AST
                        if (TYPEOF(obj) == CLOSXP && DispatchTable::check(BODY(obj))) {
                            auto rshFun = DispatchTable::unpack(BODY(obj));
                            restoreMap[obj] = BODY(obj);
                            auto origBody = src_pool_at(globalContext(), rshFun->baseline()->body()->src);
                            SET_BODY(obj, origBody);
                        }
                        #if API_DEBUG_CALLBACKS == 1
                        std::cout << i << "{ TYPE: " << TYPEOF(obj) << "} ";
                        #endif
                        SET_VECTOR_ELT(serializationObjects, i++, obj);
                    }

                    #if API_DEBUG_CALLBACKS == 1
                    std::cout << " ]" << std::endl;
                    #endif


                    #if API_DEBUG_CALLBACKS == 1
                    int j = 0;
                    std::cout << "Source Pool Entries: [ ";
                    #endif

                    for (auto & ele : spEntries) {
                        SEXP obj = src_pool_at(globalContext(), ele);
                        #if API_DEBUG_CALLBACKS == 1
                        std::cout << "{" <<  j << " at "<< i << ", TYPE: " << TYPEOF(obj) << "}" << " ";
                        j++;
                        #endif
                        SET_VECTOR_ELT(serializationObjects, i++, obj);
                    }

                    #if API_DEBUG_CALLBACKS == 1
                    std::cout << " ]" << std::endl;
                    #endif


                    #if API_DEBUG_CALLBACKS == 1
                    std::cout << "Extra Pool Entries: [ ";
                    #endif

                    for (auto & ele : ePoolEntries) {
                        #if API_DEBUG_CALLBACKS == 1
                        std::cout << i << " ";
                        #endif
                        SET_VECTOR_ELT(serializationObjects, i++, ele);
                    }

                    #if API_DEBUG_CALLBACKS == 1
                    std::cout << " ]" << std::endl;
                    #endif

                    #if API_DEBUG_CALLBACKS == 1
                    std::cout << "Promise Src Entries: [ ";
                    #endif

                    for (auto & ele : srcIndices) {
                        SEXP obj = src_pool_at(globalContext(), ele);
                        #if API_DEBUG_CALLBACKS == 1
                        std::cout << "{ " << ele  << " at " << i << "} ";
                        #endif
                        SET_VECTOR_ELT(serializationObjects, i++, obj);
                    }

                    #if API_DEBUG_CALLBACKS == 1
                    std::cout << " ]" << std::endl;
                    #endif

                    // SERIALIZE THE CONSTANT POOL
                    std::stringstream cp_stream_path;
                    cp_stream_path << hast << "_";
                    cp_stream_path << c.toI() << ".pool";

                    R_outpstream_st outputStream;
                    FILE *fptr;
                    fptr = fopen(cp_stream_path.str().c_str(),"w");
                    R_InitFileOutPStream(&outputStream,fptr,R_pstream_binary_format, 0, NULL, R_NilValue);
                    R_Serialize(serializationObjects, &outputStream);
                    fclose(fptr);

                    // Restore the original BODIES for the closures

                    for (auto & ele : restoreMap) {
                        SET_BODY(ele.first, ele.second);
                    }

                    // SERIALIZE THE LLVM MODULE
                    std::ofstream bitcodeFile;
                    std::stringstream ss;
                    ss << hast << "_";
                    ss << c.toI() << ".bc";
                    bitcodeFile.open(ss.str());
                    llvm::raw_os_ostream ooo(bitcodeFile);
                    WriteBitcodeToFile(*module,ooo);

                    #if API_PRINT_MODULE_AFTER_POOL_PATCHES == 1
                    llvm::raw_os_ostream debugOp(std::cout);
                    debugOp << *module;
                    #endif

                }
            };
            pirCompileAndSerialize(compiledFun, c, name, pir::DebugOptions::DefaultDebugOptions, serializeCallback, signatureWriter);
        }
        metaDataFile.close();
        return Rf_ScalarInteger(contextList.size());
    }

    Rf_error("Not a valid function");

    return R_FalseValue;
}


REXPORT SEXP rirMarkFunction(SEXP what, SEXP which, SEXP reopt_,
                             SEXP forceInline_, SEXP disableInline_,
                             SEXP disableSpecialization_,
                             SEXP disableArgumentTypeSpecialization_,
                             SEXP disableNumArgumentSpecialization_,
                             SEXP depromiseArgs_) {
    if (!isValidClosureSEXP(what))
        Rf_error("Not rir compiled code");
    if (TYPEOF(which) != INTSXP || LENGTH(which) != 1)
        Rf_error("index not an integer");
    auto i = INTEGER(which)[0];
    SEXP b = BODY(what);
    DispatchTable* dt = DispatchTable::unpack(b);
    if (i < 0 || (size_t)i > dt->size())
        Rf_error("version with this number does not exist");

    auto getBool = [](SEXP v) {
        if (TYPEOF(v) != LGLSXP) {
            Rf_warning("non-boolean flag");
            return NA_LOGICAL;
        }
        if (LENGTH(v) == 0)
            return NA_LOGICAL;
        return LOGICAL(v)[0];
    };

    auto reopt = getBool(reopt_);
    auto forceInline = getBool(forceInline_);
    auto disableInline = getBool(disableInline_);
    auto disableSpecialization = getBool(disableSpecialization_);
    auto disableNumArgumentSpecialization =
        getBool(disableNumArgumentSpecialization_);
    auto disableArgumentTypeSpecialization =
        getBool(disableArgumentTypeSpecialization_);
    auto depromiseArgs = getBool(depromiseArgs_);

    Function* fun = dt->get(i);
    if (reopt != NA_LOGICAL) {
        if (reopt) {
            fun->flags.set(Function::MarkOpt);
            fun->flags.reset(Function::NotOptimizable);
        } else {
            fun->flags.reset(Function::MarkOpt);
        }
    }
    if (forceInline != NA_LOGICAL) {
        if (forceInline)
            fun->flags.set(Function::ForceInline);
        else
            fun->flags.reset(Function::ForceInline);
    }
    if (disableInline != NA_LOGICAL) {
        if (disableInline)
            fun->flags.set(Function::DisableInline);
        else
            fun->flags.reset(Function::DisableInline);
    }
    if (disableSpecialization != NA_LOGICAL) {
        if (disableSpecialization)
            fun->flags.set(Function::DisableAllSpecialization);
        else
            fun->flags.reset(Function::DisableAllSpecialization);
    }
    if (disableArgumentTypeSpecialization != NA_LOGICAL) {
        if (disableArgumentTypeSpecialization)
            fun->flags.set(Function::DisableArgumentTypeSpecialization);
        else
            fun->flags.reset(Function::DisableArgumentTypeSpecialization);
    }
    if (disableNumArgumentSpecialization != NA_LOGICAL) {
        if (disableNumArgumentSpecialization)
            fun->flags.set(Function::DisableNumArgumentsSpezialization);
        else
            fun->flags.reset(Function::DisableNumArgumentsSpezialization);
    }

    bool DISABLE_ANNOTATIONS = getenv("PIR_DISABLE_ANNOTATIONS") ? true : false;
    if (!DISABLE_ANNOTATIONS) {
        if (depromiseArgs != NA_LOGICAL) {
            if (depromiseArgs)
                fun->flags.set(Function::DepromiseArgs);
            else
                fun->flags.reset(Function::DepromiseArgs);
        }
    }

    return R_NilValue;
}

REXPORT SEXP rirFunctionVersions(SEXP what) {
    if (!isValidClosureSEXP(what))
        Rf_error("Not rir compiled code");
    DispatchTable* dt = DispatchTable::unpack(BODY(what));
    auto res = Rf_allocVector(INTSXP, dt->size());
    for (size_t i = 0; i < dt->size(); ++i)
        INTEGER(res)[i] = i;
    return res;
}

REXPORT SEXP rirBody(SEXP cls) {
    if (!isValidClosureSEXP(cls))
        Rf_error("Not a valid rir compiled function");
    return DispatchTable::unpack(BODY(cls))->baseline()->container();
}

REXPORT SEXP pirDebugFlags(
#define V(n) SEXP n,
    LIST_OF_PIR_DEBUGGING_FLAGS(V)
#undef V
        SEXP IHaveTooManyCommasDummy) {
    pir::DebugOptions opts;

#define V(n)                                                                   \
    if (Rf_asLogical(n))                                                       \
        opts.flags.set(pir::DebugFlag::n);
    LIST_OF_PIR_DEBUGGING_FLAGS(V)
#undef V

    SEXP res = Rf_allocVector(INTSXP, 1);
    INTEGER(res)[0] = (int)opts.flags.to_i();
    return res;
}

static pir::DebugOptions::DebugFlags getInitialDebugFlags() {
    auto verb = getenv("PIR_DEBUG");
    if (!verb)
        return pir::DebugOptions::DebugFlags();
    std::istringstream in(verb);

    pir::DebugOptions::DebugFlags flags;
    while (!in.fail()) {
        std::string opt;
        std::getline(in, opt, ',');
        if (opt.empty())
            continue;

        bool success = false;

#define V(flag)                                                                \
    if (opt.compare(#flag) == 0) {                                             \
        success = true;                                                        \
        flags = flags | pir::DebugFlag::flag;                                  \
    }
        LIST_OF_PIR_DEBUGGING_FLAGS(V)
#undef V
        if (!success) {
            std::cerr << "Unknown PIR debug flag " << opt << "\n"
                      << "Valid flags are:\n";
#define V(flag) std::cerr << "- " << #flag << "\n";
            LIST_OF_PIR_DEBUGGING_FLAGS(V)
#undef V
            exit(1);
        }
    }
    return flags;
}

static std::regex getInitialDebugPassFilter() {
    auto filter = getenv("PIR_DEBUG_PASS_FILTER");
    if (filter)
        return std::regex(filter);
    return std::regex(".*");
}

static std::regex getInitialDebugFunctionFilter() {
    auto filter = getenv("PIR_DEBUG_FUNCTION_FILTER");
    if (filter)
        return std::regex(filter);
    return std::regex(".*");
}

static pir::DebugStyle getInitialDebugStyle() {
    auto styleStr = getenv("PIR_DEBUG_STYLE");
    if (!styleStr) {
        return pir::DebugStyle::Standard;
    }
    pir::DebugStyle style;
    if (!parseDebugStyle(styleStr, style)) {
        std::cerr << "Unknown PIR debug print style " << styleStr << "\n"
                  << "Valid styles are:\n";
#define V(style) std::cerr << "- " << #style << "\n";
        LIST_OF_DEBUG_STYLES(V)
#undef V
        exit(1);
    }
    return style;
}

pir::DebugOptions pir::DebugOptions::DefaultDebugOptions = {
    getInitialDebugFlags(), getInitialDebugPassFilter(),
    getInitialDebugFunctionFilter(), getInitialDebugStyle()};

REXPORT SEXP pirSetDebugFlags(SEXP debugFlags) {
    if (TYPEOF(debugFlags) != INTSXP || Rf_length(debugFlags) < 1)
        Rf_error(
            "pirSetDebugFlags expects an integer vector as second parameter");
    pir::DebugOptions::DefaultDebugOptions.flags =
        pir::DebugOptions::DebugFlags(INTEGER(debugFlags)[0]);
    return R_NilValue;
}

SEXP pirCompileAndSerialize(SEXP what, const Context& assumptions, const std::string& name,
                const pir::DebugOptions& debug, std::function<void(llvm::Module*, rir::Code *, std::vector<unsigned> &)> sCallback, std::function<void(FunctionSignature &, std::string)> signatureCallback) {
    if (!isValidClosureSEXP(what)) {
        Rf_error("not a compiled closure");
    }
    if (!DispatchTable::check(BODY(what))) {
        Rf_error("Cannot optimize compiled expression, only closure");
    }

    PROTECT(what);

    bool dryRun = debug.includes(pir::DebugFlag::DryRun);
    // compile to pir
    pir::Module* m = new pir::Module;
    pir::StreamLogger logger(debug);
    logger.title("Compiling " + name);
    pir::Compiler cmp(m, logger);
    pir::Backend backend(m, logger, name);

    auto compile = [&](pir::ClosureVersion* c) {
        logger.flush();
        cmp.optimizeModule();

        if (dryRun)
            return;

        rir::Function* done = nullptr;
        auto apply = [&](SEXP body, pir::ClosureVersion* c) {
            if (body == BODY(what))
                backend.addSerializer(sCallback, signatureCallback);
            auto fun = backend.getOrCompile(c);
            Protect p(fun->container());
            DispatchTable::unpack(body)->insert(fun);
            if (body == BODY(what)) {
                done = fun;
                backend.clearSerializer();
            }
        };
        m->eachPirClosureVersion([&](pir::ClosureVersion* c) {
            if (c->owner()->hasOriginClosure()) {
                auto cls = c->owner()->rirClosure();
                auto body = BODY(cls);
                auto dt = DispatchTable::unpack(body);
                if (dt->contains(c->context())) {
                    auto other = dt->dispatch(c->context());
                    assert(other != dt->baseline());
                    assert(other->context() == c->context());
                    if (other->body()->isCompiled())
                        return;
                }
                // Don't lower functions that have not been called often, as
                // they have incomplete type-feedback.
                if (dt->size() == 1 && dt->baseline()->invocationCount() < 2)
                    return;
                apply(body, c);
            }
        });
        if (!done)
            apply(BODY(what), c);
        // Eagerly compile the main function
        done->body()->nativeCode();
    };

    cmp.compileClosure(what, name, assumptions, true, compile,
                       [&]() {
                           if (debug.includes(pir::DebugFlag::ShowWarnings))
                               std::cerr << "Compilation failed\n";
                       },
                       {});

    delete m;
    UNPROTECT(1);
    return what;
}

SEXP pirCompile(SEXP what, const Context& assumptions, const std::string& name,
                const pir::DebugOptions& debug) {
    if (!isValidClosureSEXP(what)) {
        Rf_error("not a compiled closure");
    }
    if (!DispatchTable::check(BODY(what))) {
        Rf_error("Cannot optimize compiled expression, only closure");
    }

    PROTECT(what);

    bool dryRun = debug.includes(pir::DebugFlag::DryRun);
    // compile to pir
    pir::Module* m = new pir::Module;
    pir::StreamLogger logger(debug);
    logger.title("Compiling " + name);
    pir::Compiler cmp(m, logger);
    pir::Backend backend(m, logger, name);
    auto compile = [&](pir::ClosureVersion* c) {
        logger.flush();
        cmp.optimizeModule();

        if (dryRun)
            return;

        rir::Function* done = nullptr;
        auto apply = [&](SEXP body, pir::ClosureVersion* c) {
            auto fun = backend.getOrCompile(c);
            Protect p(fun->container());
            DispatchTable::unpack(body)->insert(fun);
            if (body == BODY(what))
                done = fun;
        };
        m->eachPirClosureVersion([&](pir::ClosureVersion* c) {
            if (c->owner()->hasOriginClosure()) {
                auto cls = c->owner()->rirClosure();
                auto body = BODY(cls);
                auto dt = DispatchTable::unpack(body);
                if (dt->contains(c->context())) {
                    auto other = dt->dispatch(c->context());
                    assert(other != dt->baseline());
                    assert(other->context() == c->context());
                    if (other->body()->isCompiled())
                        return;
                }
                // Don't lower functions that have not been called often, as
                // they have incomplete type-feedback.
                if (dt->size() == 1 && dt->baseline()->invocationCount() < 2)
                    return;
                apply(body, c);
            }
        });
        if (!done)
            apply(BODY(what), c);
        // Eagerly compile the main function
        done->body()->nativeCode();
    };

    cmp.compileClosure(what, name, assumptions, true, compile,
                       [&]() {
                           if (debug.includes(pir::DebugFlag::ShowWarnings))
                               std::cerr << "Compilation failed\n";
                       },
                       {});

    delete m;
    UNPROTECT(1);
    return what;
}

REXPORT SEXP rirInvocationCount(SEXP what) {
    if (!isValidClosureSEXP(what)) {
        Rf_error("not a compiled closure");
    }
    auto dt = DispatchTable::check(BODY(what));
    assert(dt);

    SEXP res = Rf_allocVector(INTSXP, dt->size());
    for (size_t i = 0; i < dt->size(); ++i)
        INTEGER(res)[i] = dt->get(i)->invocationCount();

    return res;
}

REXPORT SEXP pirCompileWrapper(SEXP what, SEXP name, SEXP debugFlags,
                               SEXP debugStyle) {
    if (debugFlags != R_NilValue &&
        (TYPEOF(debugFlags) != INTSXP || Rf_length(debugFlags) != 1))
        Rf_error(
            "pirCompileWrapper expects an integer scalar as second parameter");
    if (debugStyle != R_NilValue && TYPEOF(debugStyle) != SYMSXP)
        Rf_error("pirCompileWrapper expects a symbol as third parameter");
    std::string n;
    if (TYPEOF(name) == SYMSXP)
        n = CHAR(PRINTNAME(name));
    pir::DebugOptions opts = pir::DebugOptions::DefaultDebugOptions;

    if (debugFlags != R_NilValue) {
        opts.flags = pir::DebugOptions::DebugFlags(*INTEGER(debugFlags));
    }
    if (debugStyle != R_NilValue) {
        if (!parseDebugStyle(CHAR(PRINTNAME(debugStyle)), opts.style)) {
            Rf_error("pirCompileWrapper - given unknown debug style");
        }
    }
    return pirCompile(what, rir::pir::Compiler::defaultContext, n, opts);
}

REXPORT SEXP pirTests() {
    if (pir::Parameter::PIR_OPT_LEVEL < 2) {
        Rf_warning("pirCheck only runs with opt level 2");
        return R_FalseValue;
    }

    PirTests::run();
    return R_NilValue;
}

REXPORT SEXP pirCheckWarmupBegin(SEXP f, SEXP checksSxp, SEXP env) {
    if (oldMaxInput == 0) {
        oldMaxInput = pir::Parameter::MAX_INPUT_SIZE;
        oldInlinerMax = pir::Parameter::INLINER_MAX_SIZE;
        oldSerializeChaos = pir::Parameter::RIR_SERIALIZE_CHAOS;
        oldDeoptChaos = pir::Parameter::DEOPT_CHAOS;
    }
    pir::Parameter::MAX_INPUT_SIZE = 3500;
    pir::Parameter::INLINER_MAX_SIZE = 4000;
    pir::Parameter::RIR_SERIALIZE_CHAOS = 0;
    pir::Parameter::DEOPT_CHAOS = false;
    return R_NilValue;
}
REXPORT SEXP pirCheckWarmupEnd(SEXP f, SEXP checksSxp, SEXP env) {
    pir::Parameter::MAX_INPUT_SIZE = oldMaxInput;
    pir::Parameter::INLINER_MAX_SIZE = oldInlinerMax;
    pir::Parameter::RIR_SERIALIZE_CHAOS = oldSerializeChaos;
    pir::Parameter::DEOPT_CHAOS = oldDeoptChaos;
    return R_NilValue;
}

REXPORT SEXP pirCheck(SEXP f, SEXP checksSxp, SEXP env) {
    if (TYPEOF(checksSxp) != LISTSXP)
        Rf_error("pirCheck: 2nd parameter must be a pairlist (of symbols)");
    std::list<PirCheck::Type> checkTypes;
    for (SEXP c = checksSxp; c != R_NilValue; c = CDR(c)) {
        SEXP checkSxp = CAR(c);
        if (TYPEOF(checkSxp) != SYMSXP)
            Rf_error("pirCheck: each item in 2nd parameter must be a symbol");
        PirCheck::Type type = PirCheck::parseType(CHAR(PRINTNAME(checkSxp)));
        if (type == PirCheck::Type::Invalid)
            Rf_error("pirCheck: invalid check type. List of check types:"
#define V(Check) "\n    " #Check
                     LIST_OF_PIR_CHECKS(V)
#undef V
            );
        checkTypes.push_back(type);
    }
    // Automatically compile rir for convenience (necessary to get PIR)
    if (!isValidClosureSEXP(f))
        rirCompile(f, env);
    PirCheck check(checkTypes);
    bool res = check.run(f);
    return res ? R_TrueValue : R_FalseValue;
}

SEXP rirOptDefaultOpts(SEXP closure, const Context& assumptions, SEXP name) {
    std::string n = "";
    if (TYPEOF(name) == SYMSXP)
        n = CHAR(PRINTNAME(name));
    // PIR can only optimize closures, not expressions
    if (isValidClosureSEXP(closure))
        return pirCompile(closure, assumptions, n,
                          pir::DebugOptions::DefaultDebugOptions);
    else
        return closure;
}

SEXP rirOptDefaultOptsDryrun(SEXP closure, const Context& assumptions,
                             SEXP name) {
    std::string n = "";
    if (TYPEOF(name) == SYMSXP)
        n = CHAR(PRINTNAME(name));
    // PIR can only optimize closures, not expressions
    if (isValidClosureSEXP(closure))
        return pirCompile(
            closure, assumptions, n,
            pir::DebugOptions::DefaultDebugOptions |
                pir::DebugOptions::DebugFlags(pir::DebugFlag::DryRun));
    else
        return closure;
}

REXPORT SEXP rirSerialize(SEXP data, SEXP fileSexp) {
    oldPreserve = pir::Parameter::RIR_PRESERVE;
    pir::Parameter::RIR_PRESERVE = true;
    if (TYPEOF(fileSexp) != STRSXP)
        Rf_error("must provide a string path");
    FILE* file = fopen(CHAR(Rf_asChar(fileSexp)), "w");
    if (file == NULL)
        Rf_error("couldn't open file at path");
    R_SaveToFile(data, file, 0);
    fclose(file);
    R_Visible = (Rboolean) false;
    pir::Parameter::RIR_PRESERVE = oldPreserve;
    return R_NilValue;
}

REXPORT SEXP rirDeserialize(SEXP fileSexp) {
    oldPreserve = pir::Parameter::RIR_PRESERVE;
    pir::Parameter::RIR_PRESERVE = true;
    if (TYPEOF(fileSexp) != STRSXP)
        Rf_error("must provide a string path");
    FILE* file = fopen(CHAR(Rf_asChar(fileSexp)), "r");
    if (file == NULL)
        Rf_error("couldn't open file at path");
    SEXP res = R_LoadFromFile(file, 0);
    fclose(file);
    pir::Parameter::RIR_PRESERVE = oldPreserve;
    return res;
}

REXPORT SEXP rirEnableLoopPeeling() {
    Compiler::loopPeelingEnabled = true;
    return R_NilValue;
}

REXPORT SEXP rirDisableLoopPeeling() {
    Compiler::loopPeelingEnabled = false;
    return R_NilValue;
}

REXPORT SEXP rirPrintBuiltinIds() {
    FUNTAB* finger = R_FunTab;
    int i = 0;
    std::cout << "#ifndef RIR_BUILTIN_IDS_H\n"
              << "#define RIR_BUILTIN_IDS_H\n"
              << "// This file is generated using rir.printBuiltinIds()\n"
              << "#include \"utils/String.h\"\n"
              << "#include <cassert>\n"
              << "namespace rir {\n"
              << "static inline void errorWrongBuiltin() { "
              << "assert(false && \"wrong builtin id\"); }\n"
              << "constexpr static inline int blt(const char* name) {\n";
    while (finger->name) {
        std::cout << "    ";
        if (finger != R_FunTab)
            std::cout << "else ";
        std::cout << "if (staticStringEqual(name, \"" << finger->name
                  << "\"))\n"
                  << "        return " << i << ";\n";
        i++;
        finger++;
    }
    std::cout << "    else\n        errorWrongBuiltin();\n";
    std::cout << "    return -1;\n}\n} // namespace rir\n#endif\n";
    return R_NilValue;
}

REXPORT SEXP rirSetUserContext(SEXP f, SEXP userContext) {

    if (TYPEOF(f) != CLOSXP)
        Rf_error("f not closure");

    if (TYPEOF(BODY(f)) != EXTERNALSXP) {
        rirCompile(f, CLOENV(f));
    }

    if (TYPEOF(userContext) != INTSXP || LENGTH(userContext) != 2)
        Rf_error("userDefinedContext should be an Integer Array of size 2");

    Context newContext;
    auto p = (int*)((void*)&newContext);
    *p = INTEGER(userContext)[0];
    p++;
    *p = INTEGER(userContext)[1];

    auto tbl = DispatchTable::unpack(BODY(f));
    auto newTbl = tbl->newWithUserContext(newContext);
    SET_BODY(f, newTbl->container());
    return R_NilValue;
}

REXPORT SEXP rirCreateSimpleIntContext() {
    Context newContext = Context();
    newContext.setSimpleInt(0);

    int* p = (int*)((void*)&newContext);
    int n1 = *p;
    p++;
    int n2 = *p;

    auto res = Rf_allocVector(INTSXP, 2);
    INTEGER(res)[0] = n1;
    INTEGER(res)[1] = n2;
    return res;
}

REXPORT SEXP initializeBaseLib() {
    // for (auto & ele : libBase) {
    //     auto funSymSEXP = Rf_install(ele);
    //     auto functionSEXP = Rf_findFun(funSymSEXP, R_GlobalEnv);
    //     if (TYPEOF(functionSEXP) == CLOSXP) {
    //         Compiler::compileClosure(functionSEXP);
    //         // printAST(0,functionSEXP);
    //         std::cout << "closure found, compiling rir" << std::endl;
    //         if (DispatchTable::check(BODY(functionSEXP))) {
    //             DispatchTable * vtable = DispatchTable::unpack(BODY(functionSEXP));
    //             std::cout << "compiled: " << ele << ", hast: " << vtable->baseline()->body()->hast << std::endl;
    //         }
    //     }
    //     std::cout << "baselib: " << ele << ", sym: " << funSymSEXP << ", function: " << functionSEXP << " (" << TYPEOF(functionSEXP) << ")" << std::endl;
    // }
    return R_NilValue;
}

bool startup() {
    initializeRuntime();
    return true;
}

bool startup_ok = startup();
