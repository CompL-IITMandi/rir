#include "DispatchTable.h"

#include "utils/BitcodeLinkUtility.h"
#include "utils/WorklistManager.h"
#include "utils/Hast.h"
#include "utils/deserializerData.h"

namespace rir {

void DispatchTable::print(std::ostream& out, const int & space) const {
    assert(false && "--- L2 BROKEN FEATURE ---");
    // printSpace(space, out);
    // out << "=== Printing Dispatch table (" << this << ") ===" << std::endl;
    // // out << std::endl;
    // // out << "Full Runtime speculative context" << std::endl;
    // // Hast::printRawFeedback(this, out, space);
    // // out << std::endl;
    // for (size_t i = 1; i < size(); ++i) {
    //     SEXP funContainer = getEntry(i);

    //     if (L2Dispatch::check(funContainer)) {
    //         // L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
    //         // // auto fun = l2vt->dispatch();
    //         // printSpace(space+2, out);
    //         // out << "(" << i << "): " << l2vt->getFallback()->context() << std::endl;
    //         // l2vt->print(out, space+4);
    //     } else {
    //         auto fun = Function::unpack(funContainer);
    //         printSpace(space+2, out);
    //         out << "(" << i << ")[" << (fun->disabled() ? "D" : "") << "]: " << fun->context() << std::endl;
    //     }
    // }
}

void DispatchTable::insertL2(Function* fun) {
    assert(fun->signature().optimization !=
           FunctionSignature::OptimizationLevel::Baseline);

    int idx = negotiateSlot(fun->context());
    SEXP idxContainer = getEntry(idx);

    fun->vtab = this;

    if (idxContainer == R_NilValue) {
        Protect p;
        L2Dispatch * l2vt = L2Dispatch::create(fun->context(), p);
        l2vt->insert(fun);
        fun->l2Dispatcher = l2vt;
        setEntry(idx, l2vt->container());
    } else {
        if (Function::check(idxContainer)) {
            Protect p;
            L2Dispatch * l2vt = L2Dispatch::create(fun->context(), p);
            l2vt->setFallback(idxContainer);
            l2vt->insert(fun);
            fun->l2Dispatcher = l2vt;
            setEntry(idx, l2vt->container());
        } else if (L2Dispatch::check(idxContainer)) {
            L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);
            l2vt->insert(fun);
            fun->l2Dispatcher = l2vt;
        } else {
            Rf_error("Dispatch table L2insertion error, corrupted slot!!");
        }
    }
}

void DispatchTable::insert(Function* fun) {
    assert(fun->signature().optimization !=
            FunctionSignature::OptimizationLevel::Baseline);
    int idx = negotiateSlot(fun->context());
    SEXP idxContainer = getEntry(idx);

    fun->vtab = this;

    if (idxContainer == R_NilValue) {
        setEntry(idx, fun->container());
    } else {
        if (Function::check(idxContainer)) {
            // Already existing container, do what is meant to be done
            if (idx != 0) {
                // Remember deopt counts across recompilation to avoid
                // deopt loops
                Function * old = Function::unpack(idxContainer);
                fun->addDeoptCount(old->deoptCount());
                setEntry(idx, fun->container());
            }
        } else if (L2Dispatch::check(idxContainer)) {
            L2Dispatch * l2vt = L2Dispatch::unpack(idxContainer);
            Function * old = l2vt->getFallback();
            if (old) {
                fun->addDeoptCount(old->deoptCount());
            }
            l2vt->setFallback(fun->container());
        } else {
            Rf_error("Dispatch table insertion error, corrupted slot!!");
        }
    }
}

int DispatchTable::negotiateSlot(const Context& assumptions) {
    assert(size() > 0);
    size_t i;
    for (i = size() - 1; i > 0; --i) {
        auto funContainer = getEntry(i);
        Context currContext;

        if (L2Dispatch::check(funContainer)) {
            L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
            currContext = l2vt->getContext();
        } else {
            assert(Function::check(funContainer));
            currContext = Function::unpack(funContainer)->context();
        }

        if (currContext == assumptions) {
            // We already gave this context, dont delete it, just return the index
            return i;
        }
        if (!(assumptions < currContext)) {
            break;
        }
    }
    i++;
    assert(!containsL1Context(assumptions));
    if (size() == capacity()) {
// #ifdef DEBUG_DISPATCH
//         assert(false && "--- L2 BROKEN FEATURE ---");
//         std::cout << "Tried to insert into a full Dispatch table. Have: \n";
//         for (size_t i = 0; i < size(); ++i) {
//             auto e = getEntry(i);
//             std::cout << "* " << Function::unpack(e)->context() << "\n";
//         }
//         std::cout << "\n";
//         std::cout << "Tried to insert: " << assumptions << "\n";
//         Rf_error("dispatch table overflow");
// #endif
        // Evict one element and retry
        auto pos = 1 + (Random::singleton()() % (size() - 1));
        size_--;
        while (pos < size()) {
            setEntry(pos, getEntry(pos + 1));
            pos++;
        }
        return negotiateSlot(assumptions);
    }


    for (size_t j = size(); j > i; --j)
        setEntry(j, getEntry(j - 1));
    size_++;

    // Slot i is now available for insertion of context now
    setEntry(i, R_NilValue);
    return i;

// #ifdef DEBUG_DISPATCH
//     assert(false && "--- L2 BROKEN FEATURE ---");
//     std::cout << "Added version to DT, new order is: \n";
//     for (size_t i = 0; i < size(); ++i) {
//         auto e = getEntry(i);
//         std::cout << "* " << Function::unpack(e)->context() << "\n";
//     }
//     std::cout << "\n";
//     for (size_t i = 0; i < size() - 1; ++i) {
//         assert(get(i)->context() < get(i + 1)->context());
//         assert(get(i)->context() != get(i + 1)->context());
//         assert(!(get(i + 1)->context() < get(i)->context()));
//     }
//     assert(contains(fun->context()));
// #endif
}

Function* DispatchTable::dispatchConsideringDisabled(Context a, Function** disabledFunc, bool ignorePending) const {
    if (!a.smaller(userDefinedContext_)) {
#ifdef DEBUG_DISPATCH
        std::cout << "DISPATCH trying: " << a
                    << " vs annotation: " << userDefinedContext_ << "\n";
#endif
        Rf_error("Provided context does not satisfy user defined context");
    }

    auto outputDisabledFunc = (disabledFunc != nullptr);

    auto b = baseline();

    if (outputDisabledFunc)
        *disabledFunc = b;

    for (size_t i = 1; i < size(); ++i) {
        // auto eCon = getContext(i);
        auto funContainer = getEntry(i);
        Context currContext;

        if (L2Dispatch::check(funContainer)) {
            L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
            currContext = l2vt->getContext();
        } else {
            assert(Function::check(funContainer));
            currContext = Function::unpack(funContainer)->context();
        }
#ifdef DEBUG_DISPATCH
        std::cout << "DISPATCH trying: " << a << " vs " << currContext
                    << "\n";
#endif
        if (a.smaller(currContext)) {
            if (EventLogger::logLevel >= 2) {

                std::stringstream streamctx;
                streamctx << currContext;

                using namespace std::chrono;
                auto now = high_resolution_clock::now();
                EventLogger::logStats("dispatchTrying", "", (hast ? CHAR(PRINTNAME(hast)) : "NULL"), 0, now, streamctx.str(), nullptr, 0, "");


                std::stringstream eventDataJSON;
                eventDataJSON << "{"
                    << "\"hast\": " << "\"" << (hast ? CHAR(PRINTNAME(hast)) : "NULL")  << "\"" << ","
                    << "\"hastOffset\": " << "\"" << offsetIdx << "\"" << ","
                    << "\"inferred\": " << "\"" << a << "\"" << ","
                    << "\"context\": " << "\"" << currContext << "\"" << ","
                    << "\"vtab\": " << "\"" << this << "\""
                    << "}";

                EventLogger::logUntimedEvent(
                    "dispatchTrying",
                    eventDataJSON.str()
                );
            }

            Function * currFun;
            if (L2Dispatch::check(funContainer)) {
                L2Dispatch * l2vt = L2Dispatch::unpack(funContainer);
                currFun = l2vt->dispatch();
            } else {
                assert(Function::check(funContainer));
                currFun = Function::unpack(funContainer);
            }

            // L2 dispatcher can return a nullptr, in that case we continue looking to smaller context
            if (currFun == nullptr) continue;

            if (ignorePending || !currFun->pendingCompilation()) {

                if (!currFun->disabled()) {
                    if (EventLogger::logLevel >= 2) {
                        std::stringstream streamctx;
                        streamctx << currFun->context();

                        using namespace std::chrono;
                        auto now = high_resolution_clock::now();

                        EventLogger::logStats("functionIsEnabled", "", (hast ? CHAR(PRINTNAME(hast)) : "NULL"), 0, now, streamctx.str(), nullptr, 0, "");

                        if (currFun->l2Dispatcher) {

                            EventLogger::logStats("dispatchL2", "", (hast ? CHAR(PRINTNAME(hast)) : "NULL"), 0, now, streamctx.str(), nullptr, 0,"");


                            std::stringstream eventDataJSON;
                            eventDataJSON << "{"
                                << "\"hast\": " << "\"" << (hast ? CHAR(PRINTNAME(hast)) : "NULL")  << "\"" << ","
                                << "\"hastOffset\": " << "\"" << offsetIdx << "\"" << ","
                                << "\"inferred\": " << "\"" << a << "\"" << ","
                                << "\"function\": " << "\"" << currFun << "\"" << ","
                                << "\"context\": " << "\"" << currFun->context() << "\"" << ","
                                << "\"vtab\": " << "\"" << this << "\"" << ","
                                << "\"l2vtab\": " << "\"" << currFun->l2Dispatcher << "\"" << ","
                                << "\"l2Info\": " << "{" << currFun->l2Dispatcher->getInfo() << "}"
                                << "}";

                            EventLogger::logUntimedEvent(
                                "dispatchL2",
                                eventDataJSON.str()
                            );
                        } else {

                            std::stringstream streamctx;
                            streamctx << currFun->context();

                            using namespace std::chrono;
                            auto now = high_resolution_clock::now();
                            EventLogger::logStats("dispatch", "", (hast ? CHAR(PRINTNAME(hast)) : "NULL"), 0, now, streamctx.str(), nullptr, 0,"");



                            std::stringstream eventDataJSON;
                            eventDataJSON << "{"
                                << "\"hast\": " << "\"" << (hast ? CHAR(PRINTNAME(hast)) : "NULL")  << "\"" << ","
                                << "\"hastOffset\": " << "\"" << offsetIdx << "\"" << ","
                                << "\"inferred\": " << "\"" << a << "\"" << ","
                                << "\"function\": " << "\"" << currFun << "\"" << ","
                                << "\"context\": " << "\"" << currFun->context() << "\"" << ","
                                << "\"vtab\": " << "\"" << this << "\""
                                << "}";

                            EventLogger::logUntimedEvent(
                                "dispatch",
                                eventDataJSON.str()
                            );
                        }
                    }

                    // Dispatch selected function
                    return currFun;
                } else {

                    if (EventLogger::logLevel >= 2) {
                        std::stringstream streamctx;
                        streamctx << currFun->context();

                        using namespace std::chrono;
                        auto now = high_resolution_clock::now();

                        EventLogger::logStats("functionIsDisabled", "", (hast ? CHAR(PRINTNAME(hast)) : "NULL"), 0, now, streamctx.str(), nullptr, 0,"");
                    }

                    if (outputDisabledFunc && a == currContext)
                        *disabledFunc = currFun;
                }
            }
        }
    }

    if (EventLogger::logLevel >= 2) {



        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        EventLogger::logStats("dispatch", "", (hast ? CHAR(PRINTNAME(hast)) : "NULL"), 0, now, "baseline", nullptr, 0,"");


        std::stringstream eventDataJSON;
        eventDataJSON << "{"
            << "\"hast\": " << "\"" << (hast ? CHAR(PRINTNAME(hast)) : "NULL")  << "\"" << ","
            << "\"hastOffset\": " << "\"" << offsetIdx << "\"" << ","
            << "\"inferred\": " << "\"" << a << "\"" << ","
            << "\"function\": " << "\"" << b << "\"" << ","
            << "\"context\": " << "\"" << "baseline" << "\"" << ","
            << "\"vtab\": " << "\"" << this << "\""
            << "}";

        EventLogger::logUntimedEvent(
            "dispatch",
            eventDataJSON.str()
        );
    }

    return b;
}

} // namespace rir
