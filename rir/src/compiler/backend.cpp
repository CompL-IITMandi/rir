#include "backend.h"
#include "R/BuiltinIds.h"
#include "analysis/dead.h"
#include "compiler/analysis/cfg.h"
#include "compiler/analysis/last_env.h"
#include "compiler/analysis/reference_count.h"
#include "compiler/analysis/verifier.h"
#include "compiler/native/pir_jit_llvm.h"
#include "compiler/parameter.h"
#include "compiler/pir/pir_impl.h"
#include "compiler/pir/value_list.h"
#include "compiler/util/bb_transform.h"
#include "compiler/util/lowering/allocators.h"
#include "compiler/util/visitor.h"
#include "interpreter/instance.h"
#include "ir/CodeStream.h"
#include "ir/CodeVerifier.h"
#include "runtime/DispatchTable.h"
#include "simple_instruction_list.h"
#include "utils/FunctionWriter.h"
#include "utils/measuring.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <list>
#include <sstream>
#include <unordered_set>

#include "utils/serializerData.h"
#include "utils/DebugMessages.h"
#include "api.h"

#include "utils/SerializerFlags.h"

namespace rir {
namespace pir {

static void approximateRefcount(ClosureVersion* cls, Code* code,
                                NeedsRefcountAdjustment& refcount,
                                ClosureStreamLogger& log) {
    StaticReferenceCount refcountAnalysis(cls, code, log.out());
    refcountAnalysis();
    refcount = refcountAnalysis.getGlobalState();
}

static void approximateNeedsLdVarForUpdate(
    Code* code, std::unordered_set<Instruction*>& needsLdVarForUpdate) {

    auto apply = [&](Instruction* i, Value* vec_) {
        if (auto vec = Instruction::Cast(vec_)) {
            if (auto ld = LdVar::Cast(vec)) {
                if (auto su = i->hasSingleUse()) {
                    if (auto st = StVar::Cast(su)) {
                        if (ld->env() != st->env())
                            needsLdVarForUpdate.insert(vec);
                        else if (ld->forUpdate && ld->hasSingleUse())
                            ld->forUpdate = false;
                        return;
                    }
                    if (StVarSuper::Cast(su)) {
                        if (ld->forUpdate && ld->hasSingleUse())
                            ld->forUpdate = false;
                        return;
                    }
                }
                if (auto mk = MkEnv::Cast(ld->env()))
                    if (mk->stub && mk->arg(mk->indexOf(ld->varName)).val() !=
                                        UnboundValue::instance())
                        return;

                needsLdVarForUpdate.insert(vec);
            }
        }
    };

    Visitor::run(code->entry, [&](Instruction* i) {
        switch (i->tag) {
        // These are builtins which ignore value semantics...
        case Tag::CallBuiltin: {
            auto b = CallBuiltin::Cast(i);
            bool dotCall = b->builtinId == blt(".Call");
            if (dotCall || b->builtinId == blt("class<-")) {
                if (auto l = LdVar::Cast(
                        b->callArg(0).val()->followCastsAndForce())) {
                    static std::unordered_set<SEXP> block = {
                        Rf_install("C_R_set_slot"),
                        Rf_install("C_R_set_class")};
                    if (!dotCall || block.count(l->varName)) {
                        apply(i, l);
                    }
                }
            }
            break;
        }
        case Tag::Subassign1_1D:
        case Tag::Subassign2_1D:
        case Tag::Subassign1_2D:
        case Tag::Subassign2_2D:
            // Subassigns override the vector, even if the named count
            // is 1. This is only valid, if we are sure that the vector
            // is local, ie. vector and subassign operation come from
            // the same lexical scope.
            apply(i, i->arg(1).val()->followCastsAndForce());
            break;
        default: {}
        }
    });
    Visitor::run(code->entry, [&](Instruction* i) {
        if (auto l = LdVar::Cast(i))
            if (l->forUpdate)
                needsLdVarForUpdate.insert(l);
    });
}

static void lower(Module* module, Code* code) {
    DeadInstructions representAsReal(
        code, 1, Effects::Any(),
        DeadInstructions::IgnoreUsesThatDontObserveIntVsReal);

    Visitor::runPostChange(code->entry, [&](BB* bb) {
        auto it = bb->begin();
        while (it != bb->end()) {
            auto next = it + 1;
            if ((*it)->frameState() && !Deopt::Cast(*it))
                (*it)->clearFrameState();

            if (auto b = CallSafeBuiltin::Cast(*it)) {
                auto t = (*it)->type;
                if (b->builtinId == blt("length") &&
                    !t.isA(PirType::simpleScalarInt()) &&
                    !t.isA(PirType::simpleScalarReal())) {
                    // In the case we have an instruction that might statically
                    // return int or double, but there is no instruction that
                    // could observe the difference we might as well set the
                    // type to double to avoid boxing.
                    bool indistinguishable =
                        t.isA(PirType::simpleScalarInt() |
                              PirType::simpleScalarReal()) &&
                        representAsReal.isDead(*it);
                    if (indistinguishable) {
                        b->type = b->type & PirType::simpleScalarReal();
                    }
                }
            } else if (auto ldfun = LdFun::Cast(*it)) {
                // The guessed binding in ldfun is just used as a temporary
                // store. If we did not manage to resolve ldfun by now, we
                // have to remove the guess again, since apparently we
                // were not sure it is correct.
                if (ldfun->guessedBinding())
                    ldfun->clearGuessedBinding();
            } else if (auto ld = LdVar::Cast(*it)) {
                while (true) {
                    auto mk = MkEnv::Cast(ld->env());
                    if (mk && mk->stub && !mk->contains(ld->varName))
                        ld->env(mk->lexicalEnv());
                    else
                        break;
                }
            } else if (auto id = Identical::Cast(*it)) {
                if (auto ld1 = Const::Cast(id->arg(0).val())) {
                    if (auto ld2 = Const::Cast(id->arg(1).val())) {
                        id->replaceUsesWith(ld1->c() == ld2->c()
                                                ? (Value*)True::instance()
                                                : (Value*)False::instance());
                        next = bb->remove(it);
                    }
                }
            } else if (auto st = StVar::Cast(*it)) {
                auto mk = MkEnv::Cast(st->env());
                if (mk && mk->stub)
                    assert(mk->contains(st->varName));
            } else if (auto deopt = Deopt::Cast(*it)) {
                if (Parameter::DEBUG_DEOPTS) {
                    std::stringstream msgs;
                    msgs << "DEOPT:\n";
                    deopt->printRecursive(msgs, 3);
                    static std::vector<std::string> leak;
                    leak.push_back(msgs.str());
                    SEXP msg = Rf_mkString(leak.back().c_str());
                    static SEXP print =
                        Rf_findFun(Rf_install("cat"), R_GlobalEnv);
                    auto ldprint = module->c(print);
                    auto ldmsg = module->c(msg);
                    // Hack to silence the verifier.
                    auto ldmsg2 = new CastType(ldmsg, CastType::Downcast,
                                               PirType::any(), RType::prom);
                    it = bb->insert(it, ldmsg2) + 1;
                    it = bb->insert(it,
                                    new Call(Env::global(), ldprint, {ldmsg2},
                                             Tombstone::framestate(), 0));
                    next = it + 2;
                }
            } else if (auto expect = Assume::Cast(*it)) {
                if (expect->triviallyHolds()) {
                    next = bb->remove(it);
                } else {
                    auto expectation = expect->assumeTrue;
                    std::string debugMessage;
                    if (Parameter::DEBUG_DEOPTS) {
                        debugMessage = "DEOPT, assumption ";
                        {
                            std::stringstream dump;
                            if (auto i =
                                    Instruction::Cast(expect->condition())) {
                                dump << "\n";
                                i->printRecursive(dump, 4);
                                dump << "\n";
                            } else {
                                expect->condition()->printRef(dump);
                            }
                            debugMessage += dump.str();
                        }
                        debugMessage += " failed\n";
                    }
                    BBTransform::lowerExpect(
                        module, code, bb, it, expect, expectation,
                        expect->checkpoint()->bb()->falseBranch(),
                        debugMessage);
                    // lowerExpect splits the bb from current position. There
                    // remains nothing to process. Breaking seems more robust
                    // than trusting the modified iterator.
                    break;
                }
            }

            it = next;
        }
    });

    std::vector<BB*> dead;
    Visitor::run(code->entry, [&](BB* bb) {
        auto it = bb->begin();
        while (it != bb->end()) {
            auto next = it + 1;
            if (Checkpoint::Cast(*it)) {
                auto d = bb->deoptBranch();
                next = bb->remove(it);
                bb->convertBranchToJmp(true);
                if (d->predecessors().size() == 0) {
                    assert(d->successors().size() == 0);
                    dead.push_back(d);
                }
            } else if (auto p = Phi::Cast(*it)) {
                if (p->nargs() == 1) {
                    p->replaceUsesWith(p->arg(0).val());
                    next = bb->remove(it);
                }
            }
            it = next;
        }
    });
    for (auto bb : dead)
        delete bb;

    BBTransform::mergeRedundantBBs(code);

    // Insert Nop into all empty blocks to make life easier
    Visitor::run(code->entry, [&](BB* bb) {
        if (bb->isEmpty())
            bb->append(new Nop());
    });
}

static void toCSSA(Module* m, Code* code) {

    // For each Phi, insert copies
    BreadthFirstVisitor::run(code->entry, [&](BB* bb) {
        // TODO: move all phi's to the beginning, then insert the copies not
        // after each phi but after all phi's?
        for (auto it = bb->begin(); it != bb->end(); ++it) {
            auto instr = *it;
            if (auto phi = Phi::Cast(instr)) {

                for (size_t i = 0; i < phi->nargs(); ++i) {
                    BB* pred = phi->inputAt(i);
                    // If pred is branch insert a new split block
                    if (!pred->isJmp()) {
                        assert(pred->isBranch());
                        BB* split = nullptr;
                        if (pred->trueBranch() == phi->bb())
                            split = pred->trueBranch();
                        else if (pred->falseBranch() == phi->bb())
                            split = pred->falseBranch();
                        assert(split &&
                               "Don't know where to insert a phi input copy.");
                        pred = BBTransform::splitEdge(code->nextBBId++, pred,
                                                      split, code);
                    }
                    auto v = phi->arg(i).val();
                    auto copy = pred->insert(pred->end(), new PirCopy(v));
                    phi->arg(i).val() = *copy;
                }
                auto phiCopy = new PirCopy(phi);
                phi->replaceUsesWith(phiCopy);
                it = bb->insert(it + 1, phiCopy);
            }
        }
    });
}

bool MEASURE_COMPILER_BACKEND_PERF =
    getenv("PIR_MEASURE_COMPILER_BACKEND") ? true : false;

static Code* findFunCodeObj(std::unordered_map<Code*,std::unordered_map<Code*, std::pair<unsigned, MkArg*>>> & promMap) {
    // Identify root node
    // The code object that does not exist in any promise set is the root node
    Code * mainFunCodeObj = NULL;

    for (auto & element : promMap) {
        rir::pir::Code *curr_codeObj = element.first;
        bool trigger = false;

        // check if the current code object is a part of some other code's promises array
        for (auto & e : promMap) {
            if (e.second.count(curr_codeObj) > 0) {
                trigger = true;
                break;
            }

        }

        if (trigger == false) {
            if (mainFunCodeObj != NULL) {
                std::cout << "More than one root node is not possible, previous node: " << mainFunCodeObj  << std::endl;
                Rf_error("More than one root node is not possible");
            }
            mainFunCodeObj = curr_codeObj;
        }

    }

    if (!mainFunCodeObj) {
        for (auto & element : promMap) {
            std::cout << element.first << " : [ ";
            for (auto & prom : element.second) {
                std::cout << prom.first << " ";
            }
            std::cout << "]" << std::endl;
        }
        Rf_error("No root node found!");
    }

    return mainFunCodeObj;
}

static void updateFunctionMetas(std::vector<std::string> & relevantNames,
    std::unordered_map<std::string, unsigned> & srcDataMap,
    std::unordered_map<std::string, SEXP> & srcArgMap,
    std::unordered_map<std::string, std::vector<std::string>> & childrenData,
    std::unordered_map<std::string, int> & codeOffset,
    SEXP fNamesVec, SEXP fSrcDataVec, SEXP fArgDataVec, SEXP fChildrenData) {
    for (size_t i = 0; i < relevantNames.size(); i++) {
        Protect protecc;
        SEXP store;
        protecc(store = Rf_mkString(relevantNames[i].c_str()));
        SET_VECTOR_ELT(fNamesVec, i, store);

        auto data = getHastAndIndex(srcDataMap[relevantNames[i]]);
        unsigned calc = BitcodeLinkUtil::getSrcPoolIndexAtOffset(data.hast, data.index);
        if (calc != srcDataMap[relevantNames[i]]) {
            std::cout << "[src mismatch]: " << calc << ", " << srcDataMap[relevantNames[i]] << std::endl;
            SET_VECTOR_ELT(fSrcDataVec, i, R_NilValue);
        } else {
            Protect protecc_1;
            SEXP dd;
            protecc_1(dd = Rf_allocVector(VECSXP, 2));

            SET_VECTOR_ELT(dd, 0, data.hast);
            SET_VECTOR_ELT(dd, 1, Rf_ScalarInteger(data.index));

            SET_VECTOR_ELT(fSrcDataVec, i, dd);

        }

        SET_VECTOR_ELT(fArgDataVec, i, srcArgMap[relevantNames[i]]);

        auto children = childrenData[relevantNames[i]];

        SEXP childrenContainer;
        protecc(childrenContainer = Rf_allocVector(VECSXP, children.size()));

        for (size_t j = 0; j < children.size(); j++) {
            SET_VECTOR_ELT(childrenContainer, j, Rf_ScalarInteger(codeOffset[children[j]]));
        }

        SET_VECTOR_ELT(fChildrenData, i, childrenContainer);

    }
}

rir::Function* Backend::doCompile(ClosureVersion* cls,
                                  ClosureStreamLogger& log) {
    // TODO: keep track of source ast indices in the source pool
    // (for now, calls, promises and operators do)
    // + how to deal with inlined stuff?

    if (MEASURE_COMPILER_BACKEND_PERF)
        Measuring::startTimer("backend.cpp: lowering");

    FunctionWriter function;

    FunctionSignature signature(
        FunctionSignature::Environment::CalleeCreated,
        FunctionSignature::OptimizationLevel::Optimized);

    auto arg = cls->owner()->formals().original();
    for (size_t i = 0; i < cls->nargs(); ++i) {
        // In PIR default args are callee-handled.
        function.addArgWithoutDefault();
        signature.pushFormal(CAR(arg), TAG(arg));
        arg = CDR(arg);
    }

    assert(signature.formalNargs() == cls->nargs());
    std::unordered_map<Code*,
                       std::unordered_map<Code*, std::pair<unsigned, MkArg*>>>
        promMap;
    std::function<void(Code*)> lowerAndScanForPromises = [&](Code* c) {
        if (promMap.count(c))
            return;
        lower(module, c);
        toCSSA(module, c);
        log.CSSA(c);
#ifdef FULLVERIFIER
        Verify::apply(cls, "Error after lowering", true);
#else
#ifdef ENABLE_SLOWASSERT
        Verify::apply(cls, "Error after lowering");
#endif
#endif
        // cppcheck-suppress variableScope
        auto& pm = promMap[c];
        Visitor::run(c->entry, [&](Instruction* i) {
            if (auto mk = MkArg::Cast(i)) {
                auto p = mk->prom();
                if (!pm.count(p)) {
                    lowerAndScanForPromises(p);
                    pm[p] = {pm.size(), mk};
                }
            }
        });
    };
    lowerAndScanForPromises(cls);

    if (MEASURE_COMPILER_BACKEND_PERF) {
        Measuring::countTimer("backend.cpp: lowering");
        Measuring::startTimer("backend.cpp: pir2llvm");
    }


    std::set<SEXP> rMap;
    if (SerializerFlags::serializerEnabled && cData) {
        if (cData != nullptr) {
            jit.reqMapForCompilation = &rMap;
        }
        jit.serializerError = serializerError;
    }


    std::unordered_map<Code*, rir::Code*> done;
    std::function<rir::Code*(Code*)> compile = [&](Code* c) {
        if (done.count(c))
            return done.at(c);
        NeedsRefcountAdjustment refcount;
        approximateRefcount(cls, c, refcount, log);
        std::unordered_set<Instruction*> needsLdVarForUpdate;
        approximateNeedsLdVarForUpdate(c, needsLdVarForUpdate);
        auto res = done[c] = rir::Code::New(c->rirSrc()->src);
        // Can we do better?
        preserve(res->container());
        auto& pm = promMap.at(c);
        // Order of prms in the extra pool must equal id in promMap
        std::vector<Code*> proms(pm.size());
        for (auto p : pm)
            proms.at(p.second.first) = p.first;
        for (auto p : proms) {
            auto code = compile(p);
            if (pm.at(p).second->noReflection)
                res->flags.set(rir::Code::NoReflection);
            res->addExtraPoolEntry(code->container());
        }
        jit.compile(res, cls, c, promMap.at(c), refcount, needsLdVarForUpdate,
                    log);
        return res;
    };
    auto body = compile(cls);

    if (SerializerFlags::serializerEnabled && cData) {
        // Find the head code object
        Code * mainFunCodeObj = findFunCodeObj(promMap);
        SEXP hast = getHastAndIndex(done[mainFunCodeObj]->src).hast;

        // If lowering failed, skip further serialization process
        if (*serializerError == true) {
            DebugMessages::printSerializerMessage("(E) Lowering patches failed, skipping further serialization process.", 1);
        } else if (hast == R_NilValue) {
            *serializerError = true;
            DebugMessages::printSerializerMessage("(E) Backend, main src hast is blacklisted.", 1);
        } else {

            // 1: Generate a unique prefix [also makes it easy to keep track]
            // (to prevent collisions in the JIT when serializing and deserializing together)
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 999999);
            std::string startingUID;
            while (true) {
                std::stringstream ss;
                ss << "f_";
                ss << dis(gen);
                ss << std::hex << std::uppercase << CHAR(PRINTNAME(hast));
                ss << "_";
                ss << std::hex << std::uppercase << cls->context().toI();
                auto e = jit.JIT->lookup(ss.str() + "_0");
                if (e.takeError()) {
                    startingUID = ss.str();
                    break;
                }
            }

            // Code Object -> Handle
            std::unordered_map<Code *, std::string> processedName;

            // Handle -> source pool idx
            std::unordered_map<std::string, unsigned> srcDataMap;
            // Handle -> ArgOrdering data
            std::unordered_map<std::string, SEXP> srcArgMap;
            // Handle -> [] vector of child Handles
            std::unordered_map<std::string, std::vector<std::string>> childrenData;

            auto getProcessedName = [&](Code * c) {
                static int uid = 0;
                if (processedName.find(c) == processedName.end()) {
                    std::stringstream nn;
                    nn << startingUID << "_" << uid++;
                    std::string name = nn.str();
                    // Update the name in module to the new one
                    jit.updateFunctionNameInModule(done[c]->mName, name);
                    // Update the name for the handle
                    jit.patchFixupHandle(name, c);
                    processedName[c] = name;

                    auto data = getHastAndIndex(c->rirSrc()->src);
                    if (data.hast == R_NilValue) {
                        *serializerError = true;
                        DebugMessages::printSerializerErrors("backend, src hast is R_NilValue for src: " + std::to_string(c->rirSrc()->src) + ", index: " + std::to_string(data.index), 3);
                    }

                    // Add source pool idx
                    srcDataMap[name] = c->rirSrc()->src;

                    // Add arg ordering data
                    if (done[c]->arglistOrder() != nullptr) {
                        srcArgMap[name] = done[c]->argOrderingVec;
                    } else {
                        srcArgMap[name] = R_NilValue;
                    }

                    return name;
                } else {
                    return processedName[c];
                }
            };

            std::function<void(Code* c)>
                updateModuleNames = [&](Code* c) {
                std::string name = getProcessedName(c);
                // Traverse over all the promises for the current code object
                auto & promisesForThisObj = promMap[c];
                std::vector<std::string> children;
                // If there are promises, then work on this
                if (promisesForThisObj.size() > 0) {
                    for (size_t i = 0; i < promisesForThisObj.size(); i++) {
                        // get i'th promise
                        auto curr = done[c]->getExtraPoolEntry(i);
                        for (auto & promise : promisesForThisObj) {
                            if (curr == done[promise.first]->container()) {
                                auto nn = getProcessedName(promise.first);
                                children.push_back(nn);
                                break;
                            }
                        }
                    }
                }
                childrenData[name] = children;
            };

            // 2: Populates srcDataMap, srcArgMap, childrenData
            // If any child src is not valid, then we skip serializing
            for (auto & ele : promMap) {
                updateModuleNames(ele.first);
            }

            if (*serializerError == true) {
                DebugMessages::printSerializerMessage("(E) Failed processing promise map.", 1);
            } else {
                // Vector of native handles
                std::vector<std::string> relevantNames;
                for (auto & ele : processedName) {
                    relevantNames.push_back(ele.second);
                }

                // 3: Move the main code handle to the 0th index
                std::string mainName = getProcessedName(mainFunCodeObj);
                if (relevantNames.at(0).compare(mainName) != 0) {
                    int mainNameIndex = 0;
                    for (size_t i = 0; i < relevantNames.size(); i++) {
                        if (relevantNames.at(i).compare(mainName) == 0) {
                            mainNameIndex = i;
                            break;
                        }
                    }
                    std::swap(relevantNames[0],relevantNames[mainNameIndex]);
                }

                // 4: Keep track of which index contains which handle
                std::unordered_map<std::string, int> codeOffset;
                for (size_t i = 0; i < relevantNames.size(); i++) {
                    codeOffset[relevantNames[i]] = i;
                }

                Protect protecc;

                // Serialized Pool Data
                SEXP serializedPoolData;
                protecc(serializedPoolData = Rf_allocVector(VECSXP, SerializedPool::getStorageSize()));

                // 0 (rir::FunctionSignature) Function Signature
                // 1 (SEXP) Function Names
                // 2 (SEXP) Function Src
                // 3 (SEXP) Function Arglist Order
                // 4 (SEXP) Children Data,
                // 5 (SEXP) cPool
                // 6 (SEXP) sPool
                SEXP fNamesVec, fSrcDataVec, fArgDataVec, fChildrenData;
                protecc(fNamesVec = Rf_allocVector(VECSXP, relevantNames.size()));
                protecc(fSrcDataVec = Rf_allocVector(VECSXP, relevantNames.size()));
                protecc(fArgDataVec = Rf_allocVector(VECSXP, relevantNames.size()));
                protecc(fChildrenData = Rf_allocVector(VECSXP, relevantNames.size()));

                updateFunctionMetas(relevantNames, srcDataMap, srcArgMap, childrenData, codeOffset, fNamesVec, fSrcDataVec, fArgDataVec, fChildrenData);

                SerializedPool::addFS(serializedPoolData, signature);
                SerializedPool::addFNames(serializedPoolData, fNamesVec);
                SerializedPool::addFSrc(serializedPoolData, fSrcDataVec);
                SerializedPool::addFArg(serializedPoolData, fArgDataVec);
                SerializedPool::addFChildren(serializedPoolData, fChildrenData);

                jit.serializeModule(cData, done[mainFunCodeObj], serializedPoolData, relevantNames, mainName);

                if (DebugMessages::serializerDebugLevel() > 1) {
                    SerializedPool::print(serializedPoolData, 2);
                }

                if (*serializerError == true) {
                    DebugMessages::printSerializerMessage("(E) Serializing module failed, serializing module or pool failed.", 1);
                }

                // Entry [8]: requirement map
                std::vector<SEXP> reqMap;
                for (auto & ele : rMap) {
                    if (ele != hast) {
                        reqMap.push_back(ele);
                    }
                }

                SEXP rData;
                protecc(rData = Rf_allocVector(VECSXP, reqMap.size()));

                int i = 0;
                for (auto & ele : reqMap) {
                    SET_VECTOR_ELT(rData, i++, ele);
                }

                contextData::addReqMapForCompilation(cData, rData);

                if (DebugMessages::serializerDebugLevel() > 1) {
                    contextData::print(cData, 2);
                }
                // }
            }
        }
    }

    if (MEASURE_COMPILER_BACKEND_PERF) {
        Measuring::countTimer("backend.cpp: pir2llvm");
    }

    log.finalPIR(cls);
    function.finalize(body, signature, cls->context());
    for (auto& c : done)
        c.second->function(function.function());

    function.function()->inheritFlags(cls->owner()->rirFunction());
    return function.function();
}

Backend::LastDestructor::~LastDestructor() {
    if (MEASURE_COMPILER_BACKEND_PERF) {
        Measuring::countTimer("backend.cpp: overal");
    }
}
Backend::LastDestructor::LastDestructor() {
    if (MEASURE_COMPILER_BACKEND_PERF) {
        Measuring::startTimer("backend.cpp: overal");
    }
}

void Backend::deserializeAndPopulateBitcode(SEXP cData, SEXP hast, SEXP offsetSym, DispatchTable * vtab) {
    jit.deserializeAndPopulateBitcode(cData, hast, offsetSym, vtab);
}

rir::Function* Backend::getOrCompile(ClosureVersion* cls) {
    auto res = done.find(cls);
    if (res != done.end())
        return res->second;

    auto& log = logger.get(cls);
    done[cls] = nullptr;
    auto fun = doCompile(cls, log);
    done[cls] = fun;
    log.flush();

    return fun;
}

bool Parameter::DEBUG_DEOPTS = getenv("PIR_DEBUG_DEOPTS") &&
                               0 == strncmp("1", getenv("PIR_DEBUG_DEOPTS"), 1);

} // namespace pir
} // namespace rir
