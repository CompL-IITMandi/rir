#include "pir_jit_llvm.h"
#include "api.h"
#include "compiler/native/builtins.h"
#include "compiler/native/lower_function_llvm.h"
#include "compiler/native/pass_schedule_llvm.h"
#include "compiler/native/types_llvm.h"
#include "utils/filesystem.h"

#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/Mangling.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_os_ostream.h"
#include <memory>

#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "utils/serializerData.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "utils/FunctionWriter.h"
#include "utils/Hast.h"
#include "utils/SerializerDebug.h"
#include "R/Funtab.h"

namespace rir {
namespace pir {

std::unique_ptr<llvm::orc::LLJIT> PirJitLLVM::JIT;

size_t PirJitLLVM::nModules = 1;
bool PirJitLLVM::initialized = false;

bool LLVMDebugInfo() {
    return DebugOptions::DefaultDebugOptions.flags.contains(
        DebugFlag::LLVMDebugInfo);
}

namespace {

llvm::ExitOnError ExitOnErr;
llvm::orc::ThreadSafeContext TSC;

std::string dbgFolder;

} // namespace

void PirJitLLVM::DebugInfo::addCode(Code* c) {
    assert(!codeLoc.count(c));
    codeLoc[c] = line++;
    *log << makeName(c) << "\n";
    Visitor::run(c->entry, [&](BB* bb) {
        assert(!BBLoc.count(bb));
        BBLoc[bb] = line++;
        bb->printPrologue(log->out(), false);

        for (auto i : *bb) {
            assert(!instLoc.count(i));
            instLoc[i] = line++;
            *log << "  ";
            i->print(log->out(), false);
            *log << "\n";
        }

        line++;
        bb->printEpilogue(log->out(), false, /* always print newline */ true);
    });
    line++;
    *log << "\n";
    log->flush();
}

void PirJitLLVM::DebugInfo::initializeTypes(llvm::DIBuilder* builder) {

    UnspecifiedType = builder->createUnspecifiedType("unspecified");

    VoidType = nullptr;
    VoidPtrType = builder->createPointerType(VoidType, 64);

    IntType = builder->createBasicType("int", 32, llvm::dwarf::DW_ATE_signed);
    UIntType = builder->createBasicType("unsigned int", 32,
                                        llvm::dwarf::DW_ATE_unsigned);
    DoubleType =
        builder->createBasicType("double", 64, llvm::dwarf::DW_ATE_float);

    {
        uint32_t align = 0;

        auto sxpinfo_structType = builder->createStructType(
            CU, "sxpinfo_struct", File, 0, 64, align, (llvm::DINode::DIFlags)0,
            nullptr, llvm::DINodeArray());
        llvm::SmallVector<llvm::Metadata*, 13> sxpinfoElts = {
            builder->createMemberType(
                sxpinfo_structType, "type", File, 0, 5, align, 0,
                llvm::DINode::DIFlags::FlagBitField,
                builder->createTypedef(UIntType, "SEXPTYPE", File, 0, CU)),
            builder->createMemberType(
                sxpinfo_structType, "scalar", File, 0, 1, align, 5,
                llvm::DINode::DIFlags::FlagBitField, UIntType),
            builder->createMemberType(
                sxpinfo_structType, "obj", File, 0, 1, align, 6,
                llvm::DINode::DIFlags::FlagBitField, UIntType),
            builder->createMemberType(
                sxpinfo_structType, "alt", File, 0, 1, align, 7,
                llvm::DINode::DIFlags::FlagBitField, UIntType),
            builder->createMemberType(
                sxpinfo_structType, "gp", File, 0, 16, align, 8,
                llvm::DINode::DIFlags::FlagBitField, UIntType),
            builder->createMemberType(
                sxpinfo_structType, "mark", File, 0, 1, align, 24,
                llvm::DINode::DIFlags::FlagBitField, UIntType),
            builder->createMemberType(
                sxpinfo_structType, "debug", File, 0, 1, align, 25,
                llvm::DINode::DIFlags::FlagBitField, UIntType),
            builder->createMemberType(
                sxpinfo_structType, "trace", File, 0, 1, align, 26,
                llvm::DINode::DIFlags::FlagBitField, UIntType),
            builder->createMemberType(
                sxpinfo_structType, "spare", File, 0, 1, align, 27,
                llvm::DINode::DIFlags::FlagBitField, UIntType),
            builder->createMemberType(
                sxpinfo_structType, "gcgen", File, 0, 1, align, 28,
                llvm::DINode::DIFlags::FlagBitField, UIntType),
            builder->createMemberType(
                sxpinfo_structType, "gccls", File, 0, 3, align, 29,
                llvm::DINode::DIFlags::FlagBitField, UIntType),
            builder->createMemberType(
                sxpinfo_structType, "named", File, 0, 16, align, 32,
                llvm::DINode::DIFlags::FlagBitField, UIntType),
            builder->createMemberType(
                sxpinfo_structType, "extra", File, 0, 16, align, 48,
                llvm::DINode::DIFlags::FlagBitField, UIntType)};
        sxpinfo_structType->replaceElements(
            builder->getOrCreateArray(sxpinfoElts));

        auto SEXPRECTy = builder->createStructType(
            CU, "SEXPREC", File, 0, 448, align, (llvm::DINode::DIFlags)0,
            nullptr, llvm::DINodeArray());

        auto SEXPTy = builder->createPointerType(SEXPRECTy, 64);

        auto primsxp_structType = builder->createStructType(
            CU, "primsxp_struct", File, 0, 32, align, (llvm::DINode::DIFlags)0,
            nullptr, llvm::DINodeArray());
        llvm::SmallVector<llvm::Metadata*, 1> primsxpElts = {
            builder->createMemberType(primsxp_structType, "offset", File, 0, 32,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      IntType)};
        primsxp_structType->replaceElements(
            builder->getOrCreateArray(primsxpElts));

        auto symsxp_structType = builder->createStructType(
            CU, "symsxp_struct", File, 0, 192, align, (llvm::DINode::DIFlags)0,
            nullptr, llvm::DINodeArray());
        llvm::SmallVector<llvm::Metadata*, 3> symsxpElts = {
            builder->createMemberType(symsxp_structType, "pname", File, 0, 64,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(symsxp_structType, "value", File, 0, 64,
                                      align, 64, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(symsxp_structType, "internal", File, 0,
                                      64, align, 128, (llvm::DINode::DIFlags)0,
                                      SEXPTy)};
        symsxp_structType->replaceElements(
            builder->getOrCreateArray(symsxpElts));

        auto listsxp_structType = builder->createStructType(
            CU, "listsxp_struct", File, 0, 192, 64, (llvm::DINode::DIFlags)0,
            nullptr, llvm::DINodeArray());
        llvm::SmallVector<llvm::Metadata*, 3> listsxpElts = {
            builder->createMemberType(listsxp_structType, "carval", File, 0, 64,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(listsxp_structType, "cdrval", File, 0, 64,
                                      align, 64, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(listsxp_structType, "tagval", File, 0, 64,
                                      align, 128, (llvm::DINode::DIFlags)0,
                                      SEXPTy)};
        listsxp_structType->replaceElements(
            builder->getOrCreateArray(listsxpElts));

        auto envsxp_structType = builder->createStructType(
            CU, "envsxp_struct", File, 0, 192, align, (llvm::DINode::DIFlags)0,
            nullptr, llvm::DINodeArray());
        llvm::SmallVector<llvm::Metadata*, 3> envsxpElts = {
            builder->createMemberType(envsxp_structType, "frame", File, 0, 64,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(envsxp_structType, "enclos", File, 0, 64,
                                      align, 64, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(envsxp_structType, "hashtab", File, 0, 64,
                                      align, 128, (llvm::DINode::DIFlags)0,
                                      SEXPTy)};
        envsxp_structType->replaceElements(
            builder->getOrCreateArray(envsxpElts));

        auto closxp_structType = builder->createStructType(
            CU, "closxp_struct", File, 0, 192, align, (llvm::DINode::DIFlags)0,
            nullptr, llvm::DINodeArray());
        llvm::SmallVector<llvm::Metadata*, 3> closxpElts = {
            builder->createMemberType(closxp_structType, "formals", File, 0, 64,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(closxp_structType, "body", File, 0, 64,
                                      align, 64, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(closxp_structType, "env", File, 0, 64,
                                      align, 128, (llvm::DINode::DIFlags)0,
                                      SEXPTy)};
        closxp_structType->replaceElements(
            builder->getOrCreateArray(closxpElts));

        auto promsxp_structType = builder->createStructType(
            CU, "promsxp_struct", File, 0, 192, align, (llvm::DINode::DIFlags)0,
            nullptr, llvm::DINodeArray());
        llvm::SmallVector<llvm::Metadata*, 3> promsxpElts = {
            builder->createMemberType(promsxp_structType, "value", File, 0, 64,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(promsxp_structType, "expr", File, 0, 64,
                                      align, 64, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(promsxp_structType, "env", File, 0, 64,
                                      align, 128, (llvm::DINode::DIFlags)0,
                                      SEXPTy)};
        promsxp_structType->replaceElements(
            builder->getOrCreateArray(promsxpElts));

        auto SEXPRECUnionTy = builder->createUnionType(
            SEXPRECTy, "", File, 0, 192, align, (llvm::DINode::DIFlags)0,
            llvm::DINodeArray());
        llvm::SmallVector<llvm::Metadata*, 6> SEXPRECUnionElts = {
            builder->createMemberType(SEXPRECUnionTy, "primsxp", File, 0, 32,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      primsxp_structType),
            builder->createMemberType(SEXPRECUnionTy, "symsxp", File, 0, 192,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      symsxp_structType),
            builder->createMemberType(SEXPRECUnionTy, "listsxp", File, 0, 192,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      listsxp_structType),
            builder->createMemberType(SEXPRECUnionTy, "envsxp", File, 0, 192,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      envsxp_structType),
            builder->createMemberType(SEXPRECUnionTy, "closxp", File, 0, 192,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      closxp_structType),
            builder->createMemberType(SEXPRECUnionTy, "promsxp", File, 0, 192,
                                      align, 0, (llvm::DINode::DIFlags)0,
                                      promsxp_structType)};
        SEXPRECUnionTy->replaceElements(
            builder->getOrCreateArray(SEXPRECUnionElts));

        llvm::SmallVector<llvm::Metadata*, 5> SEXPRECElts = {
            builder->createMemberType(SEXPRECTy, "sxpinfo", File, 0, 64, align,
                                      0, (llvm::DINode::DIFlags)0,
                                      sxpinfo_structType),
            builder->createMemberType(SEXPRECTy, "attrib", File, 0, 64, align,
                                      64, (llvm::DINode::DIFlags)0, SEXPTy),
            builder->createMemberType(SEXPRECTy, "gengc_next_node", File, 0, 64,
                                      align, 128, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(SEXPRECTy, "gengc_prev_node", File, 0, 64,
                                      align, 192, (llvm::DINode::DIFlags)0,
                                      SEXPTy),
            builder->createMemberType(SEXPRECTy, "u", File, 0, 192, align, 256,
                                      (llvm::DINode::DIFlags)0,
                                      SEXPRECUnionTy)};
        SEXPRECTy->replaceElements(builder->getOrCreateArray(SEXPRECElts));

        SEXPRECType = SEXPRECTy;
        SEXPType = builder->createTypedef(SEXPTy, "SEXP", File, 0, CU);
    }

    {
        // NativeCode type is SEXP(Code*, void*, SEXP, SEXP)
        llvm::SmallVector<llvm::Metadata*, 5> EltTys = {
            SEXPType, VoidPtrType, VoidPtrType, SEXPType, SEXPType};
        NativeCodeType = builder->createSubroutineType(
            builder->getOrCreateTypeArray(EltTys));
    }
}

llvm::DIScope* PirJitLLVM::DebugInfo::getScope() {
    return LexicalBlocks.empty() ? CU : LexicalBlocks.back();
}

void PirJitLLVM::DebugInfo::emitLocation(llvm::IRBuilder<>& builder,
                                         size_t line) {
    size_t col = 1;
    llvm::DIScope* Scope = getScope();
    builder.SetCurrentDebugLocation(
        llvm::DILocation::get(Scope->getContext(), line, col, Scope));
}

void PirJitLLVM::DebugInfo::clearLocation(llvm::IRBuilder<>& builder) {
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
}

PirJitLLVM::PirJitLLVM(const std::string& name) : name(name) {
    if (!initialized)
        initializeLLVM();
}

// We have to wait to query LLVM for native code addresses until all Code's
// (including promises) are added to the Module. Hence, in the destructor,
// we need to fixup all the native pointers.
PirJitLLVM::~PirJitLLVM() {
    assert(finalized && "forgot to call finalize?");
}

void PirJitLLVM::finalize() {
    assert(!finalized);
    if (M) {
        // Should this happen before finalize or after?
        if (LLVMDebugInfo()) {
            DIB->finalize();
        }
        for (auto & global : M->getGlobalList()) {
            auto globalName = global.getName().str();
            auto pre = globalName.substr(0,6) == "copool";
            auto namc = globalName.substr(0,6) == "named_";

            if (namc || pre) {
                global.setExternallyInitialized(false);
            }
        }
        // TODO: maybe later have TSM from the start and use locking
        //       to allow concurrent compilation?
        auto TSM = llvm::orc::ThreadSafeModule(std::move(M), TSC);
        ExitOnErr(JIT->addIRModule(std::move(TSM)));
        for (auto& fix : jitFixup)
            fix.second.first->lazyCodeHandle(fix.second.second);
        nModules++;
    }
    finalized = true;
}

void PirJitLLVM::serializeModule(SEXP cData, rir::Code * code, SEXP serializedPoolData, std::vector<std::string> & relevantNames, const std::string & mainFunName, std::set<SEXP> & rMap) {
    auto prefix = getenv("PIR_SERIALIZE_PREFIX") ? getenv("PIR_SERIALIZE_PREFIX") : "bitcodes";

    std::vector<int64_t> cpEntries;
    std::vector<int64_t> spEntries;

    std::unordered_map<int64_t, int64_t> cpAlreadySeen;
    std::unordered_map<int64_t, int64_t> spAlreadySeen;

    // We clone the module because we dont want to update constant pool references in the original module
    std::unique_ptr<llvm::Module> module = llvm::CloneModule(*M.get());

    std::vector<llvm::Function *> junkFunctionList;

    // Remove junk functions, codn and clsn patches should not be needed on the deserializer ideally
    for (auto & fun: module->getFunctionList()) {
        if (fun.getName().str().substr(0,3) == "rsh") {
            junkFunctionList.push_back(&fun);
        }
        if (fun.getName().str().substr(0,2) == "f_") {
            if (std::find(relevantNames.begin(), relevantNames.end(), fun.getName().str()) == relevantNames.end()) {
                junkFunctionList.push_back(&fun);
            }
        }
    }

    for (auto & fun : junkFunctionList) {
        fun->eraseFromParent();
    }

    llvm::PassBuilder passBuilder;
    llvm::LoopAnalysisManager loopAnalysisManager(false); // true is just to output debug info
    llvm::FunctionAnalysisManager functionAnalysisManager(false);
    llvm::CGSCCAnalysisManager cGSCCAnalysisManager(false);
    llvm::ModuleAnalysisManager moduleAnalysisManager(false);

    passBuilder.registerModuleAnalyses(moduleAnalysisManager);
    passBuilder.registerCGSCCAnalyses(cGSCCAnalysisManager);
    passBuilder.registerFunctionAnalyses(functionAnalysisManager);
    passBuilder.registerLoopAnalyses(loopAnalysisManager);
    passBuilder.crossRegisterProxies(
        loopAnalysisManager, functionAnalysisManager, cGSCCAnalysisManager, moduleAnalysisManager);


    llvm::PassBuilder::OptimizationLevel lvl = llvm::PassBuilder::OptimizationLevel::O2;

    static auto optLevel = getenv("SERIALIZER_OPT");

    if (optLevel) {
        switch(optLevel[0]) {
            case '0': lvl = llvm::PassBuilder::OptimizationLevel::O0; break;
            case '1': lvl = llvm::PassBuilder::OptimizationLevel::O1; break;
            case '2': lvl = llvm::PassBuilder::OptimizationLevel::O2; break;
            case '3': lvl = llvm::PassBuilder::OptimizationLevel::O3; break;
        }
    }

    llvm::ModulePassManager modulePassManager =
        passBuilder.buildPerModuleDefaultPipeline(lvl);
    modulePassManager.run(*module, moduleAnalysisManager);

    size_t srcPoolOffset = 0;

    // Patch all CP and SRC pool entries
    int patchValue = 0;
    // Iterating over all globals
    for (auto & global : module->getGlobalList()) {
        auto pre = global.getName().str().substr(0,6) == "copool";
        auto srp = global.getName().str().substr(0,6) == "srpool";
        auto namc = global.getName().str().substr(0,6) == "named_";

        if (namc || pre || srp) {
            global.setExternallyInitialized(false);
            // global.setLinkage(llvm::GlobalValue::LinkOnceAnyLinkage);
        }

        if (srp) {
            auto con = global.getInitializer();
            if (auto * v = llvm::dyn_cast<llvm::ConstantInt>(con)) {
                // Simple constant ints
                auto val = v->getSExtValue();
                // contains replacement value, relative to serialized pool
                llvm::Constant* replacementValue;

                if (spAlreadySeen.find(val) == spAlreadySeen.end()) {
                    // If not seen, we add it to the serialization pool
                    spEntries.push_back(val);
                    // update seen map
                    spAlreadySeen[val] = srcPoolOffset;
                    replacementValue = llvm::ConstantInt::get(rir::pir::PirJitLLVM::getContext(), llvm::APInt(32, srcPoolOffset++));
                } else {
                    // If already seen, just use the previously patched value
                    replacementValue = llvm::ConstantInt::get(rir::pir::PirJitLLVM::getContext(), llvm::APInt(32, spAlreadySeen[val]));
                }

                global.setInitializer(replacementValue);
            }
        }
        // llvm::outs() << global.getName().str()  << " used at \n";
        // for (llvm::User *user : global.users()) {
        //     // if (llvm::CallBase * cbInst = llvm::dyn_cast<llvm::CallInst>(user)) {
        //     //     auto calledFun = cbInst->getCalledFunction()->getName().str();
        //     //     std::cout << global.getName().str() << " constant used at " << calledFun << std::endl;
        //     // }
        //     llvm::outs() << "   " << *user << "\n";
        // }

        // All constant pool references have a copool prefix
        if (pre) {
            llvm::raw_os_ostream ost(std::cout);
            auto con = global.getInitializer();
            if (auto * v = llvm::dyn_cast<llvm::ConstantDataArray>(con)) {
                // Constant data array
                std::vector<llvm::Constant*> patchedIndices;

                auto arrSize = v->getNumElements();

                for (unsigned int i = 0; i < arrSize; i++) {

                    // cp Index that we are referring to
                    auto val = v->getElementAsAPInt(i).getSExtValue();

                    // contains replacement value, relative to serialized pool
                    llvm::Constant* replacementValue;

                    if (cpAlreadySeen.find(val) == cpAlreadySeen.end()) {
                        // If not seen, we add it to the serialization pool
                        cpEntries.push_back(val);
                        // update seen map
                        cpAlreadySeen[val] = patchValue;
                        replacementValue = llvm::ConstantInt::get(rir::pir::PirJitLLVM::getContext(), llvm::APInt(32, patchValue++));
                    } else {
                        // If already seen, just use the previously patched value
                        replacementValue = llvm::ConstantInt::get(rir::pir::PirJitLLVM::getContext(), llvm::APInt(32, cpAlreadySeen[val]));
                    }
                    // std::cout << "[low] cPool, from: " << val << ", to: " << cpAlreadySeen[val] << std::endl;
                    patchedIndices.push_back(replacementValue);

                }

                auto ty = llvm::ArrayType::get(rir::pir::t::Int, patchedIndices.size());
                auto newInit = llvm::ConstantArray::get(ty, patchedIndices);

                global.setInitializer(newInit);
            } else if (auto * v = llvm::dyn_cast<llvm::ConstantInt>(con)) {
                // Simple constant ints
                auto val = v->getSExtValue();

                // contains replacement value, relative to serialized pool
                llvm::Constant* replacementValue;

                if (cpAlreadySeen.find(val) == cpAlreadySeen.end()) {
                    // If not seen, we add it to the serialization pool
                    cpEntries.push_back(val);
                    // update seen map
                    cpAlreadySeen[val] = patchValue;
                    replacementValue = llvm::ConstantInt::get(rir::pir::PirJitLLVM::getContext(), llvm::APInt(32, patchValue++));
                } else {
                    // If already seen, just use the previously patched value
                    replacementValue = llvm::ConstantInt::get(rir::pir::PirJitLLVM::getContext(), llvm::APInt(32, cpAlreadySeen[val]));
                }
                // std::cout << "[low] cPool, from: " << val << ", to: " << cpAlreadySeen[val] << std::endl;
                global.setInitializer(replacementValue);
            } else if (auto * v = llvm::dyn_cast<llvm::ConstantAggregateZero>(con)) {
                // Constant data array
                std::vector<llvm::Constant*> patchedIndices;

                auto arrSize = v->getNumElements();

                for (unsigned int i = 0; i < arrSize; i++) {

                    // cp Index that we are referring to
                    auto val = 0;

                    // contains replacement value, relative to serialized pool
                    llvm::Constant* replacementValue;

                    if (cpAlreadySeen.find(val) == cpAlreadySeen.end()) {
                        // If not seen, we add it to the serialization pool
                        cpEntries.push_back(val);
                        // update seen map
                        cpAlreadySeen[val] = patchValue;
                        replacementValue = llvm::ConstantInt::get(rir::pir::PirJitLLVM::getContext(), llvm::APInt(32, patchValue++));
                    } else {
                        // If already seen, just use the previously patched value
                        replacementValue = llvm::ConstantInt::get(rir::pir::PirJitLLVM::getContext(), llvm::APInt(32, cpAlreadySeen[val]));
                    }
                    // std::cout << "[low] cPool, from: " << val << ", to: " << cpAlreadySeen[val] << std::endl;
                    patchedIndices.push_back(replacementValue);

                }

                auto ty = llvm::ArrayType::get(rir::pir::t::Int, patchedIndices.size());
                auto newInit = llvm::ConstantArray::get(ty, patchedIndices);

                global.setInitializer(newInit);
            }
        }
    }

    // SERIALIZE THE LLVM MODULE
    std::stringstream bcPathSS;
    bcPathSS << prefix << "/" << contextData::getContext(cData) << ".bc";

    std::string ss = bcPathSS.str();
    llvm::StringRef bcPathRef(ss);

    unsigned MAX_RETRY = 3;
    bool success = false;
    unsigned tryCount = 0;
    while(tryCount <= MAX_RETRY) {

        std::error_code errC;
        llvm::raw_fd_ostream opStr(bcPathRef, errC);

        if (opStr.has_error()) {
            tryCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        WriteBitcodeToFile(*module,opStr);

        if (opStr.has_error()) {
            tryCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        opStr.close();
        success = true;
        break;

    }
    module.reset();
    if (!success) {
        *serializerError = true;
        SerializerDebug::infoMessage("(E) [pir_jit_llvm.cpp] writing bitcode failed", 2);
        return;
    }

    SEXP cPool, sPool;
    Protect protecc;
    protecc(cPool = Rf_allocVector(VECSXP, cpEntries.size()));
    protecc(sPool = Rf_allocVector(VECSXP, spEntries.size()));
    // std::cout << "  -- SERIALIZED POOL FOR " << mainFunName << std::endl;
    for (int i = 0; i < Rf_length(cPool); i++) {
        SEXP obj = Pool::get(cpEntries[i]);
        // std::cout << "    cpool[" << i << "]" << TYPEOF(obj) << " -- Loaded from CPOOL: " << cpEntries[i] << std::endl;
        // Dont serialize external code, we serialize the original AST
        if (TYPEOF(obj) == CLOSXP && TYPEOF(BODY(obj)) != BCODESXP) {
            *serializerError = true;
            SerializerDebug::infoMessage("(E) [pir_jit_llvm.cpp] constant pool contains CLOSXP", 2);
        }
        if (TYPEOF(obj) == 26) {
            *serializerError = true;
            SerializerDebug::infoMessage("(E) [pir_jit_llvm.cpp] constant pool contains EXTERNALSXP", 2);
        }
        if (TYPEOF(obj) == LANGSXP) {
            if (Hast::cPoolInverseMap.count(obj) > 0) {
                auto data = Hast::cPoolInverseMap[obj];

                if (Hast::blacklist.count(data.hast)) {
                    *serializerError = true;
                    SerializerDebug::infoMessage("(E) [pir_jit_llvm.cpp] serialized pool contains a leaked blacklisted LANGSXP", 2);
                } else {
                    SEXP resVec;
                    protecc(resVec = Rf_allocVector(VECSXP, 2));
                    SET_VECTOR_ELT(resVec, 0, data.hast);
                    SET_VECTOR_ELT(resVec, 1, Rf_install(std::to_string(data.offsetIndex).c_str()));

                    obj = resVec;
                    rMap.insert(VECTOR_ELT(obj, 0));
                }

            } else {
                *serializerError = true;
                SerializerDebug::infoMessage("(E) [pir_jit_llvm.cpp] serialized pool contains a leaked LANGSXP", 2);
            }

        }
        SET_VECTOR_ELT(cPool, i, obj);
    }

    for (int i = 0; i < Rf_length(sPool); i++) {
        SEXP obj = src_pool_at(spEntries[i]);
        // std::cout << "    spool[" << i << "]" << TYPEOF(obj) << std::endl;
        SET_VECTOR_ELT(sPool, i, obj);
    }

    SerializedPool::addCpool(serializedPoolData, cPool);
    SerializedPool::addSpool(serializedPoolData, sPool);

    // Serialize the poolData
    std::stringstream poolPathSS;
    poolPathSS << prefix << "/" << contextData::getContext(cData) << ".pool";

    FILE *fptr;
    fptr = fopen(poolPathSS.str().c_str(),"w");
    if (!fptr) {
        *serializerError = true;
        SerializerDebug::infoMessage("(E) [pir_jit_llvm.cpp] Writing pool failed, I/O related error", 2);
        return;
    }

    R_SaveToFile(serializedPoolData, fptr, 0);
    fclose(fptr);
}

void PirJitLLVM::updateFunctionNameInModule(std::string oldName, std::string newName) {
    if (M) {
        llvm::Function * f = M->getFunction(llvm::StringRef(oldName));
        f->setName(newName);
    } else {
        Rf_error("unable to update names in the module");
    }
}

void PirJitLLVM::patchFixupHandle(const std::string & newName, Code * code) {
    jitFixup[code].second = newName;
}

void PirJitLLVM::printModule() {
    llvm::raw_os_ostream ro(std::cout);
    ro << *M;
}

void PirJitLLVM::compile(
    rir::Code* target, ClosureVersion* closure, Code* code,
    const PromMap& promMap, const NeedsRefcountAdjustment& refcount,
    const std::unordered_set<Instruction*>& needsLdVarForUpdate,
    ClosureLog& log) {
    assert(!finalized);

    if (!M.get()) {
        M = std::make_unique<llvm::Module>("", *TSC.getContext());

        if (LLVMDebugInfo()) {

            DI = std::make_unique<DebugInfo>(dbgFolder, name);
            DIB = std::make_unique<llvm::DIBuilder>(*M);

            // Create a file stream log for this module
            DI->log = std::make_unique<FileLogStream>(DI->Folder + "/" +
                                                      DI->FileName);

            // Create the compile unit for the module.
            DI->File = DIB->createFile(DI->FileName, DI->Folder);
            DI->CU = DIB->createCompileUnit(llvm::dwarf::DW_LANG_C, DI->File,
                                            "PIR Compiler", false, "", 0);

            DI->initializeTypes(DIB.get());

            // Darwin only supports dwarf2.
            M->addModuleFlag(llvm::Module::Warning, "Dwarf Version",
                             JIT->getTargetTriple().isOSDarwin()
                                 ? 2
                                 : llvm::dwarf::DWARF_VERSION);

            // Add the current debug info version into the module.
            M->addModuleFlag(llvm::Module::Warning, "Debug Info Version",
                             llvm::DEBUG_METADATA_VERSION);
        }
    }

    if (LLVMDebugInfo()) {
        DI->addCode(code);
    }

    std::string mangledName = JIT->mangle(makeName(code));

    target->mName = mangledName;

    LowerFunctionLLVM funCompiler(
        target, mangledName, closure, code, promMap, refcount,
        needsLdVarForUpdate,
        // declare
        [&](Code* c, const std::string& name, llvm::FunctionType* signature) {
            assert(!funs.count(c));
            auto f = llvm::Function::Create(
                signature, llvm::Function::ExternalLinkage, name, *M);
            if (LLVMDebugInfo()) {
                llvm::AttrBuilder ab;
                ab.addAttribute(llvm::Attribute::get(*TSC.getContext(),
                                                     "frame-pointer", "all"));
                ab.addAttribute(llvm::Attribute::NoInline);
                ab.addAttribute(llvm::Attribute::NoMerge);
                ab.addAttribute(llvm::Attribute::NoRedZone);
                // ab.addAttribute(llvm::Attribute::OptimizeNone);
                ab.addAttribute(llvm::Attribute::UWTable);
                f->setAttributes(
                    llvm::AttributeList::get(*TSC.getContext(), ~0U, ab));
            }
            funs[c] = f;
            return f;
        },
        // getModule
        [&]() -> llvm::Module& { return *M; },
        // getFunction
        [&](Code* c) -> llvm::Function* {
            auto r = funs.find(c);
            if (r != funs.end())
                return r->second;
            return nullptr;
        },
        DI.get(), DIB.get());

    llvm::DISubprogram* SP = nullptr;
    if (LLVMDebugInfo()) {
        llvm::DIScope* FContext = DI->File;
        unsigned ScopeLine = 0;
        SP = DIB->createFunction(
            FContext, makeName(code), mangledName, DI->File,
            DI->getCodeLoc(code), DI->NativeCodeType, ScopeLine,
            llvm::DINode::FlagPrototyped,
            llvm::DISubprogram::toSPFlags(true /* isLocalToUnit */,
                                          true /* isDefinition */,
                                          false /* isOptimized */));

        funCompiler.fun->setSubprogram(SP);
        DI->LexicalBlocks.push_back(SP);
    }

    funCompiler.serializerError = serializerError;
    funCompiler.reqMap = reqMapForCompilation;

    funCompiler.compile();

    assert(jitFixup.count(code) == 0);

    if (LLVMDebugInfo()) {
        DI->LexicalBlocks.pop_back();
        DIB->finalizeSubprogram(SP);
    }

#ifndef NDEBUG
    if (llvm::verifyFunction(*funCompiler.fun, &llvm::errs())) {
        assert(false &&
               "Error in llvm::verifyFunction() called from pir_jit_llvm.cpp");
    }
#endif

    if (funCompiler.pirTypeFeedback)
        target->pirTypeFeedback(funCompiler.pirTypeFeedback);
    if (funCompiler.hasArgReordering()) {
        target->arglistOrder(ArglistOrder::New(funCompiler.getArgReordering()));
        SEXP aVec;
        R_PreserveObject(aVec = ArglistOrder::Newt(funCompiler.getArgReordering()));
        target->argOrderingVec = aVec;
    }
    // jitFixup.emplace(code, std::make_pair(target, funCompiler.fun->getName()));
    jitFixup.emplace(code, std::make_pair(target, funCompiler.fun->getName().str()));

    log.LLVMBitcode([&](std::ostream& out, bool tty) {
        bool debug = true;
        llvm::raw_os_ostream ro(out);
        if (debug) {
            // For debugging, print the whole module to see the debuginfo
            // Also comment out insn_assert in lower_function_llvm.cpp to get
            // smaller listings...
            ro << *M;
        } else {
            funCompiler.fun->print(ro, nullptr);
        }
    });
}

llvm::LLVMContext& PirJitLLVM::getContext() { return *TSC.getContext(); }

inline unsigned int stoui(const std::string& s) {
    unsigned long lresult = std::stoul(s, 0, 10);
    unsigned int result = lresult;
    if (result != lresult) Rf_error("stoui failed");
    return result;
};

void PirJitLLVM::initializeLLVM() {
    static int opaqueTrue = 1;
    if (initialized)
        return;

    using namespace llvm;
    using namespace llvm::orc;

    // Initialize LLVM
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    ExitOnErr.setBanner("PIR LLVM error: ");

    // Set some TargetMachine options
    auto JTMB = ExitOnErr(JITTargetMachineBuilder::detectHost());
    JTMB.getOptions().EnableMachineOutliner = true;
    JTMB.getOptions().EnableFastISel = true;

    // Create an LLJIT instance with custom TargetMachine builder and
    // ObjectLinkingLayer
    assert(!JIT.get());
    JIT = ExitOnErr(
        LLJITBuilder()
            .setJITTargetMachineBuilder(std::move(JTMB))
            .setObjectLinkingLayerCreator(
                [&](ExecutionSession& ES, const Triple& TT) {
                    auto GetMemMgr = []() {
                        return std::make_unique<SectionMemoryManager>();
                    };
                    auto ObjLinkingLayer =
                        std::make_unique<RTDyldObjectLinkingLayer>(
                            ES, std::move(GetMemMgr));

                    if (LLVMDebugInfo()) {
                        // Register the event debug listeners for gdb and perf.
                        ObjLinkingLayer->registerJITEventListener(
                            *JITEventListener::createGDBRegistrationListener());
#ifdef PIR_USE_PERF
                        ObjLinkingLayer->registerJITEventListener(
                            *JITEventListener::createPerfJITEventListener());
#endif

                        // Make sure the debug info sections aren't stripped.
                        ObjLinkingLayer->setProcessAllSections(true);
                    }

                    return ObjLinkingLayer;
                })
            .create());

    // Create one global ThreadSafeContext
    assert(!TSC.getContext());
    TSC = std::make_unique<LLVMContext>();

    // Set what passes to run
    JIT->getIRTransformLayer().setTransform(PassScheduleLLVM());

    // Initialize types specific to PIR and builtins
    initializeTypes(*TSC.getContext());
    NativeBuiltins::initializeBuiltins();

    // Initialize a JITDylib for builtins - these are implemented in C++ and
    // compiled when building Ř, we need to define symbols for them and
    // initialize these to the static addresses of each builtin; they are in
    // a separate dylib because they are shared by all the modules in the
    // main dylib
    auto& builtinsDL = ExitOnErr(JIT->createJITDylib("builtins"));
    JIT->getMainJITDylib().addToLinkOrder(builtinsDL);

    // Build a map of builtin names to the builtins' addresses and populate the
    // builtins dylib
    SymbolMap builtinSymbols(
        static_cast<size_t>(NativeBuiltins::Id::NUM_BUILTINS));
    NativeBuiltins::eachBuiltin([&](const NativeBuiltin& blt) {
        auto res = builtinSymbols.try_emplace(
            JIT->mangleAndIntern(blt.name),
            JITEvaluatedSymbol(pointerToJITTargetAddress(blt.fun),
                               JITSymbolFlags::Exported |
                                   JITSymbolFlags::Callable));
        assert(res.second && "duplicate builtin?");
    });

    ExitOnErr(builtinsDL.define(absoluteSymbols(builtinSymbols)));

    // Add a generator that will look for symbols in the host process.
    // This is added to the builtins dylib so that the builtins have
    // precedence
    builtinsDL.addGenerator(
        ExitOnErr(DynamicLibrarySearchGenerator::GetForCurrentProcess(
            JIT->getDataLayout().getGlobalPrefix(),
            [MainName = JIT->mangleAndIntern("main")](
                const SymbolStringPtr& Name) { return Name != MainName; })));



    // TODO this is a bit of a hack but it works: the address is stored in the
    // name. symbols starting with "ept_" are external pointers, the ones
    // starting with "efn_" are external function pointers. these must exist in
    // the host process.
    class ExtSymbolGenerator : public llvm::orc::DefinitionGenerator {
      public:
        Error tryToGenerate(LookupState& LS, LookupKind K, JITDylib& JD,
                            JITDylibLookupFlags JDLookupFlags,
                            const SymbolLookupSet& LookupSet) override {
            orc::SymbolMap NewSymbols;
            for (auto s : LookupSet) {
                auto& Name = s.first;
                auto n = (*Name).str();
                auto ept = n.substr(0, 4) == "ept_";
                auto efn = n.substr(0, 4) == "efn_";

                auto srcIdx = n.substr(0, 7) == "srcIdx_"; // srcIdx patch

                auto spe = n.substr(0, 4) == "spe_"; // Special symbols
                auto dcs = n.substr(0, 4) == "dcs_"; // Constant SEXPs from runtime
                auto gcb = n.substr(0, 4) == "gcb_"; // BUILTINSXP
                auto spe1 = n.substr(0, 5) == "spe1_"; // SPECIALSXP
                auto sym = n.substr(0, 4) == "sym_"; // Symbols

                auto gcode = n.substr(0, 4) == "cod_"; // callable pointer to builtin

                auto clos = n.substr(0, 5) == "clos_"; // Hast to container closure
                auto code = n.substr(0, 5) == "code_"; // code objs for DeoptReason
                auto vtab = n.substr(0, 5) == "vtab_"; // Hast to dispatch table

                if (vtab) {
                    auto firstDel = n.find('_');
                    auto secondDel = n.find('_', firstDel + 1);

                    auto hast = n.substr(firstDel + 1, secondDel - firstDel - 1);
                    int offsetIndex = std::stoi(n.substr(secondDel + 1));

                    rir::DispatchTable * resTab = Hast::getVtableObjectAtOffset(Rf_install(hast.c_str()), offsetIndex);

                    NewSymbols[Name] = JITEvaluatedSymbol(
                        static_cast<JITTargetAddress>(
                            reinterpret_cast<uintptr_t>(resTab->container())),
                        JITSymbolFlags::Exported | (JITSymbolFlags::None));
                } else if (code) {
                    auto firstDel = n.find('_');
                    auto secondDel = n.find('_', firstDel + 1);

                    auto hast = n.substr(firstDel + 1, secondDel - firstDel - 1);
                    int offsetIndex = std::stoi(n.substr(secondDel + 1));

                    rir::Code * resolvedCode = Hast::getCodeObjectAtOffset(Rf_install(hast.c_str()), offsetIndex);

                    NewSymbols[Name] = JITEvaluatedSymbol(
                        static_cast<JITTargetAddress>(
                            reinterpret_cast<uintptr_t>(resolvedCode)),
                        JITSymbolFlags::Exported | (JITSymbolFlags::None));

                } else if (clos) {
                    Protect protecc;
                    auto firstDel = n.find('_');
                    auto secondDel = n.find('_', firstDel + 1);

                    SEXP hastSym = Rf_install(n.substr(firstDel + 1, secondDel - firstDel - 1).c_str());
                    int debugIdx = std::stoi(n.substr(secondDel + 1));

                    SerializerDebug::infoMessage("(Rt) [pir_jit_llvm.cpp] Patching clos", 2);
                    SerializerDebug::infoMessage(n, 4);
                    if (Hast::debugMap[debugIdx] == Hast::hastMap[hastSym].clos) {
                        SerializerDebug::infoMessage("clos address match successful", 6);
                    } else {
                        SerializerDebug::infoMessage("clos address match failed", 6);
                        SerializerDebug::infoMessage("expected: " + std::to_string((unsigned long long)Hast::debugMap[debugIdx]), 8);
                        SerializerDebug::infoMessage("got: " + std::to_string((unsigned long long)Hast::hastMap[hastSym].clos), 8);
                    }

                    assert(Hast::hastMap.count(hastSym) > 0);

                    NewSymbols[Name] = JITEvaluatedSymbol(
                        static_cast<JITTargetAddress>(
                            reinterpret_cast<uintptr_t>(Hast::hastMap[hastSym].clos)),
                        JITSymbolFlags::Exported | (JITSymbolFlags::None));
                } else if (gcode) {
                    auto id = std::stoi(n.substr(4));
                    SEXP ptr;
                    assert(R_FunTab[id].eval % 10 == 1 && "Only use for BUILTINSXP");
                    if (R_FunTab[id].eval % 100 / 10 == 0)
                        ptr = Rf_install(R_FunTab[id].name)->u.symsxp.value;
                    else
                        ptr = Rf_install(R_FunTab[id].name)->u.symsxp.internal;

                    NewSymbols[Name] = JITEvaluatedSymbol(
                        static_cast<JITTargetAddress>(
                            reinterpret_cast<uintptr_t>(getBuiltin(ptr))),
                        JITSymbolFlags::Exported | (JITSymbolFlags::None));

                } else if (sym) {
                    auto constantName = n.substr(4);
                    SEXP con = Rf_install(constantName.c_str());
                    NewSymbols[Name] = JITEvaluatedSymbol(
                        static_cast<JITTargetAddress>(
                            reinterpret_cast<uintptr_t>(con)),
                        JITSymbolFlags::Exported | (JITSymbolFlags::None));
                } else if (spe1) {
                    auto firstDel = n.find('_');
                    auto secondDel = n.find('_', firstDel + 1);

                    auto index = std::stoi(n.substr(firstDel + 1, secondDel - firstDel - 1));
                    auto sym = Rf_install(R_FunTab[index].name);
                    auto spe1 = Rf_findFun(sym,R_GlobalEnv);

                    NewSymbols[Name] = JITEvaluatedSymbol(
                        static_cast<JITTargetAddress>(
                            reinterpret_cast<uintptr_t>(spe1)),
                        JITSymbolFlags::Exported | (JITSymbolFlags::None));
                } else if (gcb) {
                    auto id = std::stoi(n.substr(4));
                    SEXP ptr;
                    assert(R_FunTab[id].eval % 10 == 1 && "Only use for BUILTINSXP");
                    if (R_FunTab[id].eval % 100 / 10 == 0)
                        ptr = Rf_install(R_FunTab[id].name)->u.symsxp.value;
                    else
                        ptr = Rf_install(R_FunTab[id].name)->u.symsxp.internal;

                    NewSymbols[Name] = JITEvaluatedSymbol(
                        static_cast<JITTargetAddress>(
                            reinterpret_cast<uintptr_t>(ptr)),
                        JITSymbolFlags::Exported | (JITSymbolFlags::None));
                } else if (dcs) {
                    auto id = std::stoi(n.substr(4));
                    SEXP ptr = R_NilValue;
                    switch (id) {
                        case 100:
                            ptr = R_GlobalEnv;
                            break;
                        case 101:
                            ptr = R_BaseEnv;
                            break;
                        case 102:
                            ptr = R_BaseNamespace;
                            break;
                        case 103:
                            ptr = R_TrueValue;
                            break;
                        case 104:
                            ptr = R_NilValue;
                            break;
                        case 105:
                            ptr = R_FalseValue;
                            break;
                        case 106:
                            ptr = R_UnboundValue;
                            break;
                        case 107:
                            ptr = R_MissingArg;
                            break;
                        case 108:
                            ptr = R_LogicalNAValue;
                            break;
                        case 109:
                            ptr = R_EmptyEnv;
                            break;
                        case 110:
                            ptr = R_RestartToken;
                            break;
                        case 111:
                            ptr = R_DimSymbol;
                            break;
                        case 112:
                            ptr = R_DotsSymbol;
                            break;
                        case 113:
                            ptr = R_NamesSymbol;
                            break;
                    }
                    NewSymbols[Name] = JITEvaluatedSymbol(
                        static_cast<JITTargetAddress>(
                            reinterpret_cast<uintptr_t>(ptr)),
                        JITSymbolFlags::Exported | (JITSymbolFlags::None));
                } else if (spe) {
                    auto constantName = n.substr(4);
                    uintptr_t addr = 0;

                    if (constantName.compare("BCNodeStackTop") == 0) {
                        addr = reinterpret_cast<uintptr_t>(&R_BCNodeStackTop);
                    } else if (constantName.compare("Visible") == 0) {
                        addr = reinterpret_cast<uintptr_t>(&R_Visible);
                    } else if (constantName.compare("returnedValue") == 0) {
                        addr = reinterpret_cast<uintptr_t>(&R_ReturnedValue);
                    } else if (constantName.compare("opaqueTrue") == 0) {
                        addr = reinterpret_cast<uintptr_t>(&opaqueTrue);
                    } else if (constantName.compare("GlobalContext") == 0) {
                        addr = reinterpret_cast<uintptr_t>(&R_GlobalContext);
                    }

                    NewSymbols[Name] = JITEvaluatedSymbol(
                        static_cast<JITTargetAddress>(addr),
                        JITSymbolFlags::Exported | (JITSymbolFlags::None));
                } else if (srcIdx) {
                    Protect protecc;
                    auto firstDel = n.find('_');
                    auto secondDel = n.find('_', firstDel + 1);

                    // SerializerDebug::infoMessage("(Rt) [pir_jit_llvm.cpp] Patching srcIdx", 2);
                    // SerializerDebug::infoMessage(n, 4);
                    // SerializerDebug::infoMessage(n.substr(firstDel + 1, secondDel - firstDel - 1), 6);
                    // SerializerDebug::infoMessage(n.substr(secondDel + 1), 6);

                    SEXP hast = Rf_install(n.substr(firstDel + 1, secondDel - firstDel - 1).c_str());
                    unsigned int offsetIndex = stoui(n.substr(secondDel + 1));

                    unsigned idx = Hast::getSrcPoolIndexAtOffset(hast, offsetIndex);

                    SEXP store;
                    protecc(store = Rf_allocVector(RAWSXP, sizeof(BC::PoolIdx)));
                    BC::PoolIdx * tmp = (BC::PoolIdx *) DATAPTR(store);
                    *tmp = idx;
                    Pool::patch(Pool::makeSpace(),store);

                    NewSymbols[Name] = JITEvaluatedSymbol(
                        static_cast<JITTargetAddress>(
                            reinterpret_cast<uintptr_t>(tmp)),
                        JITSymbolFlags::Exported | (JITSymbolFlags::None));

                } else if (ept || efn) {
                    auto addrStr = n.substr(4);
                    auto addr = std::strtoul(addrStr.c_str(), nullptr, 16);
                    NewSymbols[Name] = JITEvaluatedSymbol(
                        static_cast<JITTargetAddress>(
                            reinterpret_cast<uintptr_t>(addr)),
                        JITSymbolFlags::Exported |
                            (efn ? JITSymbolFlags::Callable
                                 : JITSymbolFlags::None));
                } else {
                    std::cout << "unknown symbol " << n << "\n";
                }
            }
            if (NewSymbols.empty())
                return Error::success();

            return JD.define(absoluteSymbols(std::move(NewSymbols)));
        }
    };

    builtinsDL.addGenerator(std::make_unique<ExtSymbolGenerator>());

    if (LLVMDebugInfo()) {
        if (getenv("PIR_GDB_FOLDER")) {
            dbgFolder = getenv("PIR_GDB_FOLDER");
            clearOrCreateDirectory(dbgFolder.c_str());
        } else {
            dbgFolder = createTmpDirectory();
            std::ofstream of("./PIR_GDB_FOLDER");
            of << dbgFolder << "\n";
        }
    }

    initialized = true;
}

} // namespace pir
} // namespace rir
