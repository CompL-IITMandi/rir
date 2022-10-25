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
#include "ir/BC.h"
#include "ir/Compiler.h"
#include "utils/measuring.h"

#include <cassert>
#include <cstdio>
#include <list>
#include <memory>
#include <string>

#include "runtimePatches.h"
#include "utils/serializerData.h"
#include "utils/UMap.h"
#include <chrono>

#include "utils/BitcodeLinkUtility.h"
#include "utils/DebugMessages.h"
#include "dirent.h"

#include "utils/SerializerFlags.h"
#include "utils/DeserializerConsts.h"
#include "utils/deserializerData.h"
#include "utils/WorklistManager.h"

using namespace std::chrono;
using namespace rir;

int DebugMessages::serializerDebug = getenv("SER_DBG") ? std::stoi(getenv("SER_DBG")) : 0;
int DebugMessages::deserializerDebug = getenv("DES_DBG") ? std::stoi(getenv("DES_DBG")) : 0;

rir::Code* DebugCheckpoints::lastCheckpoint = nullptr;
unsigned DebugCheckpoints::checkpoint = 0;
unsigned DebugCheckpoints::BCidx = 0;
bool DebugCheckpoints::enabled = getenv("DEBUG_CHECKPOINTS") ? true : false;
std::chrono::time_point<std::chrono::high_resolution_clock> DebugCheckpoints::lastCheckpointTime = high_resolution_clock::now();
std::chrono::time_point<std::chrono::high_resolution_clock> DebugCheckpoints::lastInstructionTime = high_resolution_clock::now();

#define PRINT_DEPENDENCY_MAP 0
#define PRINT_DESERIALIZER_PROGRESS 0
#define PRINT_DESERIALIZER_PROGRESS_OVERRIDE 0
#define CREATE_DOT_GRAPH 0
extern "C" Rboolean R_Visible;

int R_ENABLE_JIT = getenv("R_ENABLE_JIT") ? atoi(getenv("R_ENABLE_JIT")) : 3;

static size_t oldMaxInput = 0;
static size_t oldInlinerMax = 0;
static bool oldPreserve = false;
static unsigned oldSerializeChaos = false;
static bool oldDeoptChaos = false;

static size_t timeInPirCompiler = 0;
static size_t compilerSuccesses = 0;
static size_t metadataLoadTime = 0;
static int serializerSuccess = 0;


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

        if (!t->isL2(entry)) {
            Function* f = t->get(entry);
            std::cout << "= version " << entry << " (" << f << ") =\n";
            f->disassemble(std::cout);
        } else {
            t->disassembleL2(entry, std::cout);
        }
    }

    return R_NilValue;
}

// PRINT AST, small debugging utility
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
        printAST(space, value);
        // std::cout << "}" << std::endl;
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
    REnvHandler envObj(envSXP);
    space += 2;
    envObj.iterate([&] (SEXP key, SEXP val){
        printSpace(space);
        std::cout << CHAR(PRINTNAME(key)) << " : " << TYPEOF(val) << std::endl;
    });
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

std::vector<SEXP> currentStack;
long unsigned int maxStackSize = 10;

void printAST(int space, SEXP ast) {
    if (currentStack.size() >= maxStackSize) {
        std::cout << "}(LIMIT " << maxStackSize << ")" << std::endl;
        return;
    }
    if (std::find(currentStack.begin(), currentStack.end(), ast) != currentStack.end()) {
        std::cout << "REC...}" << std::endl;
        return;
    }
    currentStack.push_back(ast);
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
    currentStack.pop_back();
}

hastAndIndex getHastAndIndex(unsigned src, bool constantPool) {
    REnvHandler srcToHastMap(SRC_HAST_MAP);
    SEXP srcSym;
    if (constantPool) {
        srcSym = Rf_install((std::to_string(src) + "_cp").c_str());
    } else {
        srcSym = Rf_install(std::to_string(src).c_str());
    }
    if (SEXP r = srcToHastMap.get(srcSym)) {
        SEXP hastS = VECTOR_ELT(r, 0);
        SEXP indexS = VECTOR_ELT(r, 1);
        int index = std::stoi(CHAR(PRINTNAME(indexS)));
        hastAndIndex res = { hastS, index };
        // std::cout << "fet_hast_entry[" << CHAR(PRINTNAME(srcSym)) << "]:" << "(" << CHAR(PRINTNAME(hastS)) << "," << index << ")" << std::endl;
        return res;
    } else {
        // std::cout << "fet_hast_entry[" << CHAR(PRINTNAME(srcSym)) << "]: FAIL"<< std::endl;
        hastAndIndex res = { R_NilValue, 0 };
        return res;
    }
}


static void loadMetadata(std::string metaDataPath) {
    REnvHandler vtabMap(HAST_VTAB_MAP);
    Protect protecc;
    // Disable contextual compilation during deserialization
    // otherwise unnecessary evals can lead to a redundant compilations
    bool oldVal = BitcodeLinkUtil::contextualCompilationSkip;
    BitcodeLinkUtil::contextualCompilationSkip = true;

    FILE *reader;
    reader = fopen(metaDataPath.c_str(),"r");

    if (!reader) {
        DebugMessages::printDeserializerErrors("Unable to open meta for deserialization" + metaDataPath, 0);
        return;
    }

    SEXP ddContainer;
    protecc(ddContainer= R_LoadFromFile(reader, 0));

    fclose(reader);

    GeneralWorklist::insert(ddContainer);


    DebugMessages::printDeserializerMessage("loaded bitcode metadata for : " + metaDataPath, 0);
    if (DebugMessages::deserializerDebugLevel() > 1) {
        deserializerData::print(ddContainer, 2);
    }
    // If the function already exists, then try linking it right now
    auto currHast = deserializerData::getHast(ddContainer);

    if (SEXP vtab = vtabMap.get(currHast)) {
        assert(DispatchTable::check(vtab) != nullptr);
        BitcodeLinkUtil::tryLinking(DispatchTable::unpack(vtab), currHast);
    }

    BitcodeLinkUtil::contextualCompilationSkip = oldVal;
}

REXPORT SEXP printGeneralWorklist() {
    GeneralWorklist::print(2);
    return R_NilValue;
}

REXPORT SEXP loadBitcodes(SEXP pathToBc) {
    bool success = true;
    auto start = high_resolution_clock::now();

    if (DeserializerConsts::bitcodesLoaded) {
        Rf_error("Bitcodes already loaded, only allowed to load from one location in a session!");
    }

    if (pathToBc != R_NilValue) {
        assert(TYPEOF(pathToBc) == STRSXP);
        DeserializerConsts::bitcodesPath = CHAR(STRING_ELT(pathToBc, 0));
    }

    Protect prot;
    DIR *dir;
    struct dirent *ent;

    int metasFound = 0;

    if ((dir = opendir (DeserializerConsts::bitcodesPath)) != NULL) {

        while ((ent = readdir (dir)) != NULL) {
            std::string fName = ent->d_name;
            if (fName.find(".metad") != std::string::npos) {
                metasFound++;
                loadMetadata(std::string(DeserializerConsts::bitcodesPath) + "/" + fName);
            }
        }

        closedir (dir);

        if (metasFound > 0) {
            DeserializerConsts::bitcodesLoaded = true;
        }


    } else {
        DebugMessages::printDeserializerErrors("unable to open bitcodes directory", 0);
        success = false;

    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    metadataLoadTime = duration.count();
    if (success) {
        return Rf_mkString("Load Bitcodes Success");
    } else {
        return Rf_mkString("Load Bitcodes Failed");
    }

}

REXPORT SEXP rirCompile(SEXP what, SEXP env) {
    //
    // Is there a better place to do this? this is kind of a hack we have for now
    //
    static bool initializeBitcodes = false;
    if (!initializeBitcodes && DeserializerConsts::earlyBitcodes) {
        loadBitcodes(R_NilValue);
        initializeBitcodes = true;
    }
    if (TYPEOF(what) == CLOSXP) {
        SEXP body = BODY(what);
        if (TYPEOF(body) == EXTERNALSXP)
            return what;

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

REXPORT SEXP startCapturingStats() {
    SerializerFlags::captureCompileStats = true;
    return R_NilValue;
}

REXPORT SEXP stopCapturingStats() {
    SerializerFlags::captureCompileStats = false;
    return R_NilValue;
}

REXPORT SEXP compileStats(SEXP name, SEXP path) {
    assert(TYPEOF(path) == STRSXP);
    assert(TYPEOF(name) == STRSXP);
    std::ofstream ostrm(CHAR(STRING_ELT(path, 0)));
    ostrm << "============== RUN STATS ==============" << std::endl;
    ostrm << "Name                     : " << CHAR(STRING_ELT(name, 0)) << std::endl;
    ostrm << "Metadata Load Time       : " << metadataLoadTime << "ms" << std::endl;
    ostrm << "Bitcode load/link time   : " << BitcodeLinkUtil::linkTime << "ms" << std::endl;
    ostrm << "Time in PIR Compiler     : " << timeInPirCompiler << "ms" << std::endl;
    ostrm << "Compiled Closures:       : " << compilerSuccesses << std::endl;
    ostrm << "Serialized Closures      : " << serializerSuccess << std::endl;
    ostrm << "Unlinked BC (Worklist1)  : " << Worklist1::worklist.size() << std::endl;
    ostrm << "Unlinked BC (Worklist2)  : " << Worklist2::worklist.size() << std::endl;

    if (Worklist1::worklist.size() > 0) {
        ostrm << "=== Worklist 1 ===" << std::endl;
        for (auto & ele : Worklist1::worklist) {
            ostrm << CHAR(PRINTNAME(ele.first)) << " : [";
            for(auto & uEleIdx : ele.second) {
                ostrm << uEleIdx << ", ";
                // UnlockingElement::print(Pool::get(uEleIdx), 2);
            }
            ostrm << "]" << std::endl;
        }
    }

    if (Worklist2::worklist.size() > 0) {
        ostrm << "=== Worklist 2 ===" << std::endl;
        for (auto & ele : Worklist2::worklist) {
            ostrm << CHAR(PRINTNAME(ele.first)) << " : [";
            for(auto & uEleIdx : ele.second) {
                ostrm << uEleIdx << ", ";
                // OptUnlockingElement::print(Pool::get(uEleIdx), 2);
            }
            ostrm << "]" << std::endl;
        }
    }

    return R_NilValue;
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


REXPORT SEXP startSerializer() {
    SerializerFlags::serializerEnabled = true;
    return R_NilValue;
}

REXPORT SEXP stopSerializer() {
    SerializerFlags::serializerEnabled = false;
    return R_NilValue;
}

REXPORT SEXP startDebugMessages() {
    DebugCheckpoints::startDebugMessages();
    return R_NilValue;
}

REXPORT SEXP stopDebugMessages() {
    DebugCheckpoints::stopDebugMessages();
    return R_NilValue;
}

static bool fileExists(std::string fName) {
    std::ifstream f(fName.c_str());
    return f.good();
}

static void serializeClosure(SEXP hast, const unsigned & indexOffset, const std::string & name, SEXP cData, bool & serializerError) {
    Protect protecc;
    DebugMessages::printSerializerMessage("(*) serializeClosure start", 1);
    auto prefix = getenv("PIR_SERIALIZE_PREFIX") ? getenv("PIR_SERIALIZE_PREFIX") : "bitcodes";
    std::stringstream fN;
    fN << prefix << "/" << "m_" << CHAR(PRINTNAME(hast)) << ".meta";
    std::string fName = fN.str();

    SEXP sDataContainer;

    int onlyLast = getenv("ONLY_LAST") ? std::stoi(getenv("ONLY_LAST")) : 0;


    if (onlyLast == 0 && fileExists(fName)) {
        DebugMessages::printSerializerMessage("(*) metadata already exists", 2);

        FILE *reader;
        reader = fopen(fName.c_str(),"r");

        if (!reader) {
            serializerError = true;
            DebugMessages::printSerializerMessage("(*) serializeClosure failed, unable to open existing metadata", 1);
            return;
        }

        SEXP result;
        protecc(result= R_LoadFromFile(reader, 0));

        sDataContainer = result;

        //
        // Correct captured name if this is the parent closure i.e. offset == 0
        //

        if (indexOffset == 0) {
            serializerData::addName(sDataContainer, Rf_install(name.c_str()));
        }

        fclose(reader);
    } else {
        protecc(sDataContainer = Rf_allocVector(VECSXP, serializerData::getStorageSize()));

        serializerData::addHast(sDataContainer, hast);

        //
        // Temporarily store last stored inner functions name, until parent is added
        //

        std::stringstream nameStr;
        nameStr << "Inner(" << indexOffset << "): " << name << std::endl;

        serializerData::addName(sDataContainer, Rf_install(nameStr.str().c_str()));
    }
    // Add context data
    std::string offsetStr(std::to_string(indexOffset));
    SEXP offsetSym = Rf_install(offsetStr.c_str());

    std::string conStr(std::to_string(contextData::getContext(cData)));
    SEXP contextSym = Rf_install(conStr.c_str());

    SEXP epoch = serializerData::addBitcodeData(sDataContainer, offsetSym, contextSym, cData);

    DebugMessages::printSerializerMessage(std::string("current epoch: ") + std::string(CHAR(PRINTNAME(epoch))), 2);


    // 2. Write updated metadata
    FILE *fptr;
    fptr = fopen(fName.c_str(),"w");
    if (!fptr) {
        serializerError = true;
        DebugMessages::printSerializerMessage("(*) serializeClosure failed, unable to write metadata", 1);
        return;
    }

    R_SaveToFile(sDataContainer, fptr, 0);
    fclose(fptr);

    if (DebugMessages::serializerDebugLevel() > 1) {
        serializerData::print(sDataContainer, 2);
    }

    // rename temp files
    {
        std::stringstream bcFName;
        std::stringstream bcOldName;
        bcFName << prefix << "/" << CHAR(PRINTNAME(hast)) << "_" << indexOffset << "_" << CHAR(PRINTNAME(epoch)) << ".bc";
        bcOldName << prefix << "/" << contextData::getContext(cData) << ".bc";
        int stat = std::rename(bcOldName.str().c_str(), bcFName.str().c_str());
        if (stat != 0) {
            serializerError = true;
            DebugMessages::printSerializerMessage("(*) serializeClosure failed, unable to rename bitcode.", 1);
            return;
        }
    }

    {
        std::stringstream bcFName;
        std::stringstream bcOldName;
        bcFName << prefix << "/" << CHAR(PRINTNAME(hast)) << "_" << indexOffset << "_" << CHAR(PRINTNAME(epoch)) << ".pool";
        bcOldName << prefix << "/" << contextData::getContext(cData) << ".pool";
        int stat = std::rename(bcOldName.str().c_str(), bcFName.str().c_str());
        if (stat != 0) {
            serializerError = true;
            DebugMessages::printSerializerMessage("(*) serializeClosure failed, unable to rename pool.", 1);
            return;
        }
    }

}

SEXP pirCompile(SEXP what, const Context& assumptions, const std::string& name,
                const pir::DebugOptions& debug) {
    auto startCompileTimeCounter = high_resolution_clock::now();
    if (!isValidClosureSEXP(what)) {
        Rf_error("not a compiled closure");
    }
    if (!DispatchTable::check(BODY(what))) {
        Rf_error("Cannot optimize compiled expression, only closure");
    }

    // auto currDispatch = DispatchTable::unpack(BODY(what))->dispatch(assumptions);
    // if (currDispatch->context() == assumptions && !currDispatch->disabled()) {
    //     std::cout << "WEIRD CASE!" << std::endl;
    //     return what;
    // }

    PROTECT(what);

    bool dryRun = debug.includes(pir::DebugFlag::DryRun);
    // compile to pir
    pir::Module* m = new pir::Module;
    pir::StreamLogger logger(debug);
    logger.title("Compiling " + name);

    // DispatchTable::unpack(BODY(what))->baseline()->disassemble(std::cout);

    // BitcodeLinkUtil::printValidLookupIndices(DispatchTable::unpack(BODY(what)));

    // std::cout << "CC Context: " << assumptions << std::endl;
    // DispatchTable::unpack(BODY(what))->printWithAll();
    // std::cout << "Dispatch Table: " << DispatchTable::unpack(BODY(what)) << std::endl;
    // DispatchTable::unpack(BODY(what))->printWithAll();

    pir::Compiler cmp(m, logger);
    pir::Backend backend(m, logger, name);
    auto compile = [&](pir::ClosureVersion* c) {
        logger.flush();
        cmp.optimizeModule();

        if (dryRun)
            return;

        rir::Function* done = nullptr;
        auto apply = [&](SEXP body, pir::ClosureVersion* c) {

            if (SerializerFlags::serializerEnabled) {
                backend.cData = nullptr;
                backend.serializerError = nullptr;

                auto data = getHastAndIndex(c->rirSrc()->src);
                SEXP hast = data.hast;

                if (hast != R_NilValue) {
                    Protect protecc;
                    DebugMessages::printSerializerMessage("> Serializer Started", 0);
                    // Disable further compilations due to the recomipile heuristic, weird eval problems can happen
                    // when serializing/deserializing
                    bool oldVal = BitcodeLinkUtil::contextualCompilationSkip;
                    BitcodeLinkUtil::contextualCompilationSkip = true;

                    // Context data container
                    SEXP cDataContainer;
                    protecc(cDataContainer = Rf_allocVector(VECSXP, contextData::getStorageSize()));

                    contextData::addContext(cDataContainer, c->context().toI());

                    // Add the metadata collectors to the backend
                    bool serializerError = false;
                    backend.cData = cDataContainer;
                    backend.serializerError = &serializerError;

                    // Compile
                    auto fun = backend.getOrCompile(c);
                    protecc(fun->container());

                    DispatchTable::unpack(body)->insert(fun);
                    if (body == BODY(what)) {
                        done = fun;
                    }

                    if (!serializerError) {
                        serializeClosure(hast, data.index, c->name(), cDataContainer, serializerError);
                        if (!serializerError) {
                            if (SerializerFlags::captureCompileStats) serializerSuccess++;
                            DebugMessages::printSerializerMessage("/> Serializer Success", 0);
                        } else {
                            DebugMessages::printSerializerMessage("/> Serializer Error, I/O related failure", 0);
                        }
                    } else {
                        DebugMessages::printSerializerMessage("/> Serializer Error", 0);
                    }
                    backend.cData = nullptr;
                    backend.serializerError = nullptr;

                    // Restore compilations to existing state
                    BitcodeLinkUtil::contextualCompilationSkip = oldVal;
                } else {
                    // hast in null, cannot serialize
                    DebugMessages::printSerializerMessage("*> Serializer Skipped, parent hast is null.", 0);
                    auto fun = backend.getOrCompile(c);
                    Protect p(fun->container());
                    DispatchTable::unpack(body)->insert(fun);
                    if (body == BODY(what))
                        done = fun;
                }
            } else {
                auto fun = backend.getOrCompile(c);
                Protect p(fun->container());
                DispatchTable::unpack(body)->insert(fun);
                if (body == BODY(what))
                    done = fun;
            }
        };
        // bool delayMain = false;
        m->eachPirClosureVersion([&](pir::ClosureVersion* c) {
            if (c->owner()->hasOriginClosure()) {
                auto cls = c->owner()->rirClosure();
                auto body = BODY(cls);
                auto dt = DispatchTable::unpack(body);
                if (dt->contains(c->context())) {
                    //
                    // With L2 this check may fail as dispatch may be purposefully delayed
                    //
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
                // // Delay applying main
                // if (BODY(what) == body) {
                //     PROTECT(body);
                //     delayMain = true;
                //     return;
                // }
                PROTECT(body);
                apply(body, c);
                UNPROTECT(1);
                if (SerializerFlags::captureCompileStats) compilerSuccesses++;
            }
        });
        // if (delayMain) {
        //     // Fixes a big where lowering main function before recursively compiled ones leads to sub optimal
        //     // LLVM code for the main function. This is because the recursively compiled closures are yet to
        //     // be added to the dispatch table. [StaticCall lowering in the lower_function_llvm.cpp]
        //     // Also,
        //     // Cases where this is false would be interesting to look at.
        //     auto body = BODY(what);
        //     apply(body, c);
        //     UNPROTECT(1);
        // }
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
    logger.title("Compiled " + name);
    delete m;
    UNPROTECT(1);
    auto stopCompileTimeCounter = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stopCompileTimeCounter - startCompileTimeCounter);
    if (SerializerFlags::captureCompileStats) timeInPirCompiler += duration.count();
    return what;
}

static bool isHastBlacklisted(SEXP hastSym) {
    SEXP blMap = Pool::get(BL_MAP);
    if (blMap != R_NilValue && Rf_findVarInFrame(blMap, hastSym) != R_UnboundValue) {
        return true;
    } else {
        return false;
    }
}

void serializerCleanup() {
    SEXP blMap = Pool::get(BL_MAP);
    if (blMap == R_NilValue) {
        return;
    }

    auto prefix = getenv("PIR_SERIALIZE_PREFIX") ? getenv("PIR_SERIALIZE_PREFIX") : "bitcodes";

    std::stringstream savePath;
    savePath << prefix << "/";

    DIR *dir;
    struct dirent *ent;


    if ((dir = opendir (savePath.str().c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            std::string fName = ent->d_name;
            if (fName.find(".meta") != std::string::npos) {
                Protect protecc;
                std::stringstream metaPath;
                metaPath << prefix << "/" << fName;

                FILE *reader;
                reader = fopen(metaPath.str().c_str(),"r");

                if (!reader) {
                    DebugMessages::printSerializerMessage("(*) serializer cleanup failed", 1);
                    continue;
                }

                SEXP result;
                protecc(result= R_LoadFromFile(reader, 0));
                fclose(reader);

                // check if the currentHast is blacklisted
                SEXP hast = serializerData::getHast(result);

                if (isHastBlacklisted(hast)) {
                    const int removeRes = remove(metaPath.str().c_str());
                    if( removeRes != 0 ){
                        std::cerr << "[Warning] Failed to remove: " << metaPath.str() << ", res: " << removeRes << std::endl;
                    }
                    continue;
                }

                // Todo, blacklist specific contexts instead of the whole file...
                bool err = false;
                serializerData::iterate(result, [&](SEXP offsetSym, SEXP conSym, SEXP cData, bool isMask) {
                    if (!isMask) {
                        auto rData = contextData::getReqMapAsVector(cData);
                        for (int i = 0; i < Rf_length(rData); i++) {
                            SEXP dep = VECTOR_ELT(rData, i);
                            if (isHastBlacklisted(dep)) {
                                err = true;
                            }
                        }
                    }
                });

                if (err) {
                    const int removeRes = remove(metaPath.str().c_str());
                    if( removeRes != 0 ){
                        std::cerr << "Failed to remove: " << metaPath.str() << ", res: " << removeRes << std::endl;
                    }
                }
            }
        }
        closedir (dir);
    }
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

REXPORT SEXP rirResetMeasuring(SEXP outputOld) {
    if (TYPEOF(outputOld) != LGLSXP) {
        Rf_warning("non-boolean flag");
        return R_NilValue;
    }
    if (LENGTH(outputOld) == 0) {
        return R_NilValue;
    }
    Measuring::reset(LOGICAL(outputOld)[0]);
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

bool startup() {
    initializeRuntime();
    #if RESERVE_SPACES_AT_STARTUP == 1
    Pool::makeSpace(); // (1) SRC_HAST_MAP: For src to hast map
    Pool::makeSpace(); // (2) HAST_VTAB_MAP: Hast to vtable map
    Pool::makeSpace(); // (3) HAST_CLOS_MAP: Hast to closObj
    Pool::makeSpace(); // (4) BL_MAP: Hast blacklist, discard serialized code for these functions
    #endif
    return true;
}

bool startup_ok = startup();
