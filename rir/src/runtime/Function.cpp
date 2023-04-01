#include "Function.h"
#include "R/Serialize.h"
#include "compiler/compiler.h"
#include "runtime/DispatchTable.h"
#include "utils/Hast.h"
namespace rir {

bool Function::matchSpeculativeContext() {
    assert(speculativeContextIdx != -1);
    SEXP speculativeContext = body()->getExtraPoolEntry(speculativeContextIdx);
    for (int i = 0; i < Rf_length(speculativeContext); i++) {
        SEXP ele = VECTOR_ELT(speculativeContext, i);
        SpeculativeContextValue * sVal = getTFromContainerPointer<SpeculativeContextValue>(ele, 0);
        SpeculativeContextPointer * sPtr = getTFromContainerPointer<SpeculativeContextPointer>(ele, 1);

        switch (sVal->tag) {
            case 0: {
                ObservedValues* feedback = (ObservedValues*)(sPtr->pc + 1);
                uint32_t storedVal = *((uint32_t*) feedback);
                if (storedVal != sVal->uIntVal) return false;
                break;
            }
            case 1: {
                ObservedTest* feedback = (ObservedTest*)(sPtr->pc + 1);
                uint32_t storedVal = *((uint32_t*) feedback);
                if (storedVal != sVal->uIntVal) return false;
                break;
            }
            case 2: {
                SEXP val = sVal->sexpVal;
                ObservedCallees * feedback = (ObservedCallees *) (sPtr->pc + 1);

                if (TYPEOF(val) == INTSXP && INTEGER(val)[0] > 0) {
                    // Check specialsxp
                    if (feedback->invalid) return false;
                    if (feedback->numTargets == 0) return false;
                    bool match = false;
                    for (int j = 0; j < feedback->numTargets; ++j) {
                        auto target = feedback->getTarget(sPtr->code, j);
                        if (TYPEOF(target) == SPECIALSXP) {
                            match = target->u.primsxp.offset == INTEGER(val)[0];
                        }
                    }
                    if (!match) return false;
                } else if (TYPEOF(val) == SYMSXP) {
                    // Check callee
                    if (feedback->invalid) return false;
                    if (feedback->numTargets == 0) return false;
                    bool match = false;
                    for (int j = 0; j < feedback->numTargets; ++j) {
                        auto target = feedback->getTarget(sPtr->code, j);
                        if (TYPEOF(target) == CLOSXP && TYPEOF(BODY(target)) == EXTERNALSXP && DispatchTable::check(BODY(target))) {
                            auto targetVtab = DispatchTable::unpack(BODY(target));
                            auto targetCode = targetVtab->baseline()->body();

                            auto hastInfo = Hast::getHastInfo(targetCode->src, true);
                            if (hastInfo.isValid()) {
                                match = hastInfo.hast == val;
                            }
                        }
                    }
                    if (!match) return false;
                } else {
                    // std::cerr << "TYPEOF(val): " << TYPEOF(val) << std::endl;
                    assert(TYPEOF(val) == INTSXP);
                    // return false;
                }
                break;
            }
            case 3: {
                uint32_t runtimeValue = sPtr->code->flags.to_i();
                if (runtimeValue != sVal->uIntVal) return false;
                break;
            }
            case 4: {
                uint32_t runtimeValue = sPtr->code->function()->flags.to_i();
                if (runtimeValue != sVal->uIntVal) return false;
                break;
            }
            default:
                assert(false && "Speculative dispatch error!");
        }
    }
    return true;
}

void Function::printSpeculativeContext(std::ostream& out, const int & space) {
    assert(speculativeContextIdx != -1);
    SEXP speculativeContext = body()->getExtraPoolEntry(speculativeContextIdx);
    for (int i = 0; i < Rf_length(speculativeContext); i++) {
        printSpace(space, out);
        out << "Slot[" << i << "/" << (Rf_length(speculativeContext) - 1) << "]" << std::endl;
        SEXP ele = VECTOR_ELT(speculativeContext, i);
        SpeculativeContextValue * sVal = getTFromContainerPointer<SpeculativeContextValue>(ele, 0);
        SpeculativeContextPointer * sPtr = getTFromContainerPointer<SpeculativeContextPointer>(ele, 1);
        if (sVal->tag == 0) {
            ObservedValues* feedback = (ObservedValues*)(sPtr->pc + 1);
            uint32_t storedVal = *((uint32_t*) feedback);
            printSpace(space+2,out);
            out << "TAG[0]: " << sVal->uIntVal << "," << storedVal << std::endl;
        } else if (sVal->tag == 1) {
            ObservedTest* feedback = (ObservedTest*)(sPtr->pc + 1);
            uint32_t storedVal = *((uint32_t*) feedback);
            printSpace(space+2,out);
            out << "TAG[1]: " << sVal->uIntVal << "," << storedVal << std::endl;
        } else {
            SEXP val = sVal->sexpVal;
            ObservedCallees * feedback = (ObservedCallees *) (sPtr->pc + 1);

            if (TYPEOF(val) == INTSXP && INTEGER(val)[0] > 0) {
                printSpace(space+2,out);
                out << "TAG[2:Special]" << std::endl;

                // Check specialsxp
                if (feedback->invalid) {
                    printSpace(space+4,out);
                    out << "[Invalid Feedback]" << std::endl;
                    continue;
                }
                if (feedback->numTargets == 0) {
                    printSpace(space+4, out);
                    out << "[targets]" << std::endl;
                    continue;
                }
                for (int j = 0; j < feedback->numTargets; ++j) {
                    printSpace(space+4, out);
                    out << "Callee(" << j <<")" << std::endl;

                    auto target = feedback->getTarget(sPtr->code, j);
                    if (TYPEOF(target) == SPECIALSXP) {
                        printSpace(space+6, out);
                        out << "[" << target->u.primsxp.offset << "," << INTEGER(val)[0] << "]" << std::endl;
                    } else {
                        printSpace(space+6, out);
                        out << "[NONSPECIAL]" << std::endl;
                    }
                }
            } else if (TYPEOF(val) == SYMSXP) {
                printSpace(space+2,out);
                out << "TAG[2:Special]" << std::endl;

                // Check callee
                if (feedback->invalid) {
                    printSpace(space+4, out);
                    out << "[Invalid Feedback]" << std::endl;
                    continue;
                }
                if (feedback->numTargets == 0) {
                    printSpace(space+4, out);
                    out << "[targets]" << std::endl;
                    continue;
                }
                for (int j = 0; j < feedback->numTargets; ++j) {
                    printSpace(space+4, out);
                    out << "Callee(" << j <<")" << std::endl;

                    auto target = feedback->getTarget(sPtr->code, j);
                    if (TYPEOF(target) == CLOSXP && TYPEOF(BODY(target)) == EXTERNALSXP && DispatchTable::check(BODY(target))) {
                        auto targetVtab = DispatchTable::unpack(BODY(target));
                        auto targetCode = targetVtab->baseline()->body();

                        auto hastInfo = Hast::getHastInfo(targetCode->src, true);
                        if (hastInfo.isValid()) {
                            printSpace(space+6, out);
                            out << "[" << CHAR(PRINTNAME(hastInfo.hast)) << "," << CHAR(PRINTNAME(val)) << "]" << std::endl;
                        } else {
                            printSpace(space+6, out);
                            out << "[INVALID-HAST]" << std::endl;

                        }
                    } else {
                        printSpace(space+6, out);
                        out << "[NONCLOS]" << std::endl;

                    }
                }
            } else {
                printSpace(space+2,out);
                out << "[OTHER:" << TYPEOF(val) << "]" << std::endl;
            }
        }
    }
}

void Function::registerDeopt() {
    // Deopt counts are kept on the optimized versions
    assert(isOptimized());

    if (l2Dispatcher) {
        l2Dispatcher->lastDispatch.valid = false;
        l2Dispatcher->lastDispatch.fun = nullptr;
    }

    if (!disabled()) {
        flags.set(Flag::Deopt);
        if (l2Dispatcher) {
            if (EventLogger::logLevel >= 2) {
                std::stringstream eventDataJSON;
                eventDataJSON << "{"
                    << "\"hast\": " << "\"" << CHAR(PRINTNAME(vtab->hast))  << "\"" << ","
                    << "\"hastOffset\": " << "\"" << vtab->offsetIdx << "\"" << ","
                    << "\"function\": " << "\"" << this << "\"" << ","
                    << "\"context\": " << "\"" << context() << "\"" << ","
                    << "\"vtab\": " << "\"" << vtab << "\"" << ","
                    << "\"l2Info\": " << "{" << l2Dispatcher->getInfo() << "}"
                    << "}";

                EventLogger::logUntimedEvent(
                    "deoptL2",
                    eventDataJSON.str()
                );
            }

        } else if (vtab) {
            if (EventLogger::logLevel >= 2) {
                std::stringstream eventDataJSON;
                eventDataJSON << "{"
                    << "\"hast\": " << "\"" << (vtab->hast ? CHAR(PRINTNAME(vtab->hast)) : "NULL")  << "\"" << ","
                    << "\"hastOffset\": " << "\"" << vtab->offsetIdx << "\"" << ","
                    << "\"context\": " << "\"" << context() << "\"" << ","
                    << "\"function\": " << "\"" << this << "\"" << ","
                    << "\"vtab\": " << "\"" << vtab << "\""
                    << "}";

                EventLogger::logUntimedEvent(
                    "deopt",
                    eventDataJSON.str()
                );
            }
        }
        // else {
            // Case of dummy deopt sentinals
        // }
    }

    flags.set(Flag::Deopt);

    if (deoptCount_ < UINT_MAX)
        deoptCount_++;

}

void Function::registerDeoptReason(DeoptReason::Reason r) {
    // Deopt reasons are counted in the baseline
    assert(!isOptimized());
    if (r == DeoptReason::DeadCall)
        deadCallReached_++;
    if (r == DeoptReason::EnvStubMaterialized)
        flags.set(NeedsFullEnv);
}

void Function::addFastcaseInvalidationConditions(LastDispatchFastcaseHolder * f) {
    assert(speculativeContextIdx != -1);
    SEXP speculativeContext = body()->getExtraPoolEntry(speculativeContextIdx);

    for (int i = 0; i < Rf_length(speculativeContext); i++) {
        SEXP ele = VECTOR_ELT(speculativeContext, i);
        SpeculativeContextPointer * sPtr = getTFromContainerPointer<SpeculativeContextPointer>(ele, 1);

        if (sPtr->pc) {
            Hast::l2FastcaseInvalidationCache[sPtr->pc + 1].insert(f);
        }
    }
}
Function* Function::deserialize(SEXP refTable, R_inpstream_t inp) {
    size_t functionSize = InInteger(inp);
    const FunctionSignature sig = FunctionSignature::deserialize(refTable, inp);
    const Context as = Context::deserialize(refTable, inp);
    SEXP store = Rf_allocVector(EXTERNALSXP, functionSize);
    void* payload = DATAPTR(store);
    Function* fun = new (payload) Function(functionSize, nullptr, {}, sig, as);
    fun->numArgs_ = InInteger(inp);
    fun->info.gc_area_length += fun->numArgs_;
    for (unsigned i = 0; i < fun->numArgs_ + 1; i++) {
        fun->setEntry(i, R_NilValue);
    }
    PROTECT(store);
    AddReadRef(refTable, store);
    SEXP body = Code::deserialize(refTable, inp)->container();
    fun->body(body);
    PROTECT(body);
    int protectCount = 2;
    for (unsigned i = 0; i < fun->numArgs_; i++) {
        if ((bool)InInteger(inp)) {
            SEXP arg = Code::deserialize(refTable, inp)->container();
            PROTECT(arg);
            protectCount++;
            fun->setEntry(Function::NUM_PTRS + i, arg);
        } else
            fun->setEntry(Function::NUM_PTRS + i, nullptr);
    }
    // fun->flags = EnumSet<Flag>(InInteger(inp));
    UNPROTECT(protectCount);
    return fun;
}

void Function::serialize(SEXP refTable, R_outpstream_t out) const {
    OutInteger(out, size);
    signature().serialize(refTable, out);
    context_.serialize(refTable, out);
    OutInteger(out, numArgs_);
    HashAdd(container(), refTable);
    body()->serialize(refTable, out);
    for (unsigned i = 0; i < numArgs_; i++) {
        Code* arg = defaultArg(i);
        OutInteger(out, (int)(arg != nullptr));
        if (arg)
            defaultArg(i)->serialize(refTable, out);
    }
    OutInteger(out, flags.to_i());
}

void Function::disassemble(std::ostream& out) {
    out << "[sigature] ";
    signature().print(out);
    if (!context_.empty())
        out << "| context: [" << context_ << "]";
    out << "\n";
    out << "[flags]    ";
#define V(F)                                                                   \
    if (flags.includes(F))                                                     \
        out << #F << " ";
    RIR_FUNCTION_FLAGS(V)
#undef V
    out << "\n";
    out << "[stats]    ";
    out << "invoked: " << invocationCount()
        << ", time: " << ((double)invocationTime() / 1e6)
        << "ms, deopt: " << deoptCount();
    out << "\n";
    body()->disassemble(out);
}

static int GLOBAL_SPECIALIZATION_LEVEL =
    getenv("PIR_GLOBAL_SPECIALIZATION_LEVEL")
        ? atoi(getenv("PIR_GLOBAL_SPECIALIZATION_LEVEL"))
        : 100;
void Function::clearDisabledAssumptions(Context& given) const {
    if (flags.contains(Function::DisableArgumentTypeSpecialization))
        given.clearTypeFlags();
    if (flags.contains(Function::DisableNumArgumentsSpezialization))
        given.clearNargs();
    if (flags.contains(Function::DisableAllSpecialization))
        given.clearExcept(pir::Compiler::minimalContext);

    if (GLOBAL_SPECIALIZATION_LEVEL < 100)
        given.setSpecializationLevel(GLOBAL_SPECIALIZATION_LEVEL);
}

} // namespace rir
