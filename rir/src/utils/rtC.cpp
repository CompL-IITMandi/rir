#include "rtC.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include "interpreter/call_context.h"
#include <iostream>
#include <algorithm>
#include <set>
#include <fstream>
#include <functional>
#include <chrono>
#include <ctime>
#include "../compiler/pir/closure_version.h"
#include <bits/stdc++.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <numeric>

namespace rir {

	namespace {
		using namespace std;

		// unordered_map<size_t, unique_ptr<FunLabel>> names; // names: either a string, or an id for anonymous functions
        unordered_map<size_t, std::string> names;

        static int OPTIMISM = 5;
        static int FAIRNESS = 5;

		class DispatchData {
			public:
                double call_count = 0;
                double success_run_count = 0;

                double compilation_time = 0;

                double run_time = 0;

                double bb_count = 0;
                double p_count = 0;

                double forced_count = 0;
                double deopt_count = 0;
                double forced_runtime = 0;

                double averageForcedRuntime() {
                    return forced_runtime/forced_count;
                }

                double averageRuntime() {
                    return run_time/success_run_count;
                }

                double successRate() {
                    return (success_run_count/(call_count - forced_count))*100;
                }

                bool calculable() {
                    return (success_run_count >= FAIRNESS && forced_count >= OPTIMISM);
                }

                bool isGoodForAot() {
                    return (
                        averageRuntime() < averageForcedRuntime()
                    );
                }

                int jitBreakEvenPoint() {
                    return (
                        (compilation_time/(averageForcedRuntime() - averageRuntime())) + 1
                    );
                }

                bool isGoodForJit(int under) {
                    return (
                        ((averageRuntime()*under) + compilation_time) < (averageForcedRuntime()*under)
                    );
                }

                double speedup() {
                    return (
                        averageForcedRuntime() / averageRuntime()
                    );
                }
		};

		class Entry {
			public:
                unordered_map<Context, DispatchData> dispatch_data_holder;

                int successful_compilations = 0;
                int failed_compilations = 0;

                double total_call_count = 0;

		};

		// Map from a function (identified by its body) to the data about the
		// different contexts it has been called in
		unordered_map<size_t, Entry> entries;
        std::unordered_map<std::string, bool> function_map;

        unsigned int total_successful_compilations = 0;
        unsigned int total_failed_compilations = 0;

		struct FileLogger {
            double total_runtime = 0;
            ofstream rt_stream, log_stream;
            const char * DELIMITER = ",";
			static size_t getEntryKey(SEXP callee) {
				/* Identify a function by the SEXP of its BODY. For nested functions, The
				   enclosing CLOSXP changes every time (because the CLOENV also changes):
				       f <- function {
				         g <- function() { 3 }
						 g()
					   }
					Here the BODY of g is always the same SEXP, but a new CLOSXP is used
					every time f is called.
				*/
				return reinterpret_cast<size_t>(BODY(callee));
			}

			void createEntry(CallContext const& call) {
                // function id
				auto fun_id = getEntryKey(call.callee);
				// create or get entry
				auto & entry = entries[fun_id];
                entry.total_call_count++;
			}

			void addDispatchInfo(
                std::string funName,
                size_t fun_id,
                double runtime,
                Context ctxt,
                bool tragedy,
                bool deopt
			) {
                names[fun_id] = funName;
				// Create or get entry
                auto & entry = entries[fun_id];

                total_runtime += runtime;

                // Total call count for the dispatched context
                entry.dispatch_data_holder[ctxt].call_count++;

                // Is a function running in baseline due to some reason ? -> tragedy
                if(tragedy) {
                    if(deopt) entry.dispatch_data_holder[ctxt].deopt_count++;
                    entry.dispatch_data_holder[ctxt].forced_count++;
                    entry.dispatch_data_holder[ctxt].forced_runtime += runtime;
                } else {
                    entry.dispatch_data_holder[ctxt].success_run_count++;
                    entry.dispatch_data_holder[ctxt].run_time += runtime;
                }
			}

            void saveCompilationData(
				SEXP callee,
				Context compiled_context,
				double time,
				int bb,
				int p
			) {
                // get the entry for this function
                auto id = getEntryKey(callee);
				// create or get entry
				auto & entry = entries[id];

                total_successful_compilations++;

                entry.successful_compilations++;

                entry.dispatch_data_holder[compiled_context].compilation_time += time;
                entry.dispatch_data_holder[compiled_context].bb_count = bb;
                entry.dispatch_data_holder[compiled_context].p_count = p;
            }

            void registerFailedCompilation(SEXP callee) {
                // get the entry for this function
                auto id = getEntryKey(callee);
				// create or get entry
				auto & entry = entries[id];
                entry.failed_compilations++;
                total_failed_compilations++;
            }

            void saveFunctionMap(std::unordered_map<std::string, bool> map) {
                // save the loaded function map from the rtBin files
                function_map = map;
            }
            template<typename T>
            void LOG(std::stringstream & ss, T & msg_unit) {
                if (getenv("RT_LOG")) {
                    ss << msg_unit << DELIMITER;
                } else {
                    std::cout << msg_unit << DELIMITER;
                }
            }

            void LOG_NEXT(std::stringstream & ss) {
                if (getenv("RT_LOG")) {
                    ss << std::endl;
                } else {
                    std::cout << std::endl;
                }
            }

            void LOG_END_TUPLE(std::stringstream & ss) {
                if (getenv("RT_LOG")) {
                    log_stream << ss.str();
                } else {
                    std::cout << ss.str();
                }
            }

			~FileLogger() {
                // Save the log to a file ?
                if(getenv("RT_LOG") != NULL) {
					string binId = getenv("SAVE_BIN");
					log_stream.open(binId + ".csv");
				}
                // Save/Update rtBin Files ?
                if(getenv("SAVE_BIN") != NULL) {
					string binId = getenv("SAVE_BIN");
					rt_stream.open(binId + ".rtBin");
                    for (auto & itr : function_map) {
                        string key = itr.first;
                        rt_stream << key << std::endl;
                    }
				}
                // HEADER MESSAGE
                if (getenv("RT_LOG")) {
                    log_stream
                        << "ID"
                        << DELIMITER
                        << "NAME"
                        << DELIMITER
                        << "CALL COUNT"
                        << DELIMITER
                        << "CONTEXT"
                        << DELIMITER
                        << "CTXT CALL COUNT"
                        << DELIMITER
                        << "FORCED COUNT"
                        << DELIMITER
                        << "SUCCESS COUNT"
                        << DELIMITER
                        << "SUCCESS RATE"
                        << DELIMITER
                        << "JIT BREAK EVEN COUNT"
                        << DELIMITER
                        << "AVG FORCED_TIME"
                        << DELIMITER
                        << "AVG RUNTIME"
                        << DELIMITER
                        << std::endl;
                } else {
                    std::cout
                        << "ID"
                        << DELIMITER
                        << "NAME"
                        << DELIMITER
                        << "CALL COUNT"
                        << DELIMITER
                        << "CONTEXT"
                        << DELIMITER
                        << "CTXT CALL COUNT"
                        << DELIMITER
                        << "FORCED COUNT"
                        << DELIMITER
                        << "SUCCESS COUNT"
                        << DELIMITER
                        << "SUCCESS RATE"
                        << DELIMITER
                        << "JIT BREAK EVEN COUNT"
                        << DELIMITER
                        << "AVG FORCED_TIME"
                        << DELIMITER
                        << "AVG RUNTIME"
                        << DELIMITER
                        << std::endl;
                }

                set<string> data_interp;
                for (auto & itr : entries) {
                    string & name = names[itr.first];
                    Entry & entry = itr.second;

                    if (name.compare("") == 0) continue; // skip over anonymous functions
                    if (entry.dispatch_data_holder.size() == 0) continue; // skip over functions with no specialization

                    stringstream tuple_stream;
                    std::unordered_map<int, int> bBMap;

                    // reserve enough positions already to prevent copying
                    bBMap.reserve(entry.dispatch_data_holder.size());

                    // LOG BASIC DATA
                    LOG(tuple_stream, itr.first);  // ID
                    LOG(tuple_stream, name);  // NAME
                    LOG(tuple_stream, entry.total_call_count); // CALL COUNT

                    bool first_printed = false;
                    bool similar_binaries = false;

                    set<string> candidates;

                    for (auto & con : entry.dispatch_data_holder) {
                        auto & dispatched = con.second;
                        double jit_break_even_point = dispatched.jitBreakEvenPoint();
                        double dispatched_avg_runtime = dispatched.averageRuntime();
                        double dispatched_avg_forcetime = dispatched.averageForcedRuntime();
                        string success_rate = std::to_string(dispatched.successRate()) + "%";

                        string forced_deopt = (std::to_string(dispatched.forced_count) + " (" + std::to_string(dispatched.deopt_count) + ")");

                        stringstream context_stream;
                        context_stream << con.first;

                        std::string context = std::to_string(((Context)con.first).toI()) + ":" + context_stream.str();
                        size_t pos;
                        while ((pos = context.find(",")) != std::string::npos) {
                            context.replace(pos, 1, " ");
                        }

                        while ((pos = context.find(";")) != std::string::npos) {
                            context.replace(pos, 1, " - ");
                        }

                        if(dispatched.bb_count > 1) bBMap[dispatched.bb_count]++;

                        if(first_printed) { LOG(tuple_stream, "^"); LOG(tuple_stream, "^"); LOG(tuple_stream, "^"); } // leave first few blocks empty
                        LOG(tuple_stream, context); // CONTEXT

                        LOG(tuple_stream, dispatched.call_count); // CALL COUNT
                        LOG(tuple_stream, forced_deopt); // FORCED COUNT (DEOPT COUNT)
                        LOG(tuple_stream, dispatched.success_run_count); // SUCCESSFUL RUN COUNT

                        LOG(tuple_stream, success_rate); // SUCCESS RATE

                        // JIT BREAK EVEN COUNT
                        if(jit_break_even_point > 0) {
                            LOG(tuple_stream, jit_break_even_point);
                        } else {
                            LOG(tuple_stream, "NOJIT");
                        }

                        // AVERAGE FORCETIME
                        LOG(tuple_stream, dispatched_avg_forcetime);

                        // AVERAGE RUNTIME
                        LOG(tuple_stream, dispatched_avg_runtime);

                        first_printed = true;

                        if(getenv("SAVE_BIN") != NULL && dispatched.calculable()) {
                            // if an entry is not good for AOT
                            if(!dispatched.isGoodForAot())
                                // skip if the entry is already accounted for
                                if(function_map.find(name + std::to_string(((Context)con.first).toI())) == function_map.end())
                                    data_interp.insert((name + std::to_string(((Context)con.first).toI())));
                        }
                        LOG_NEXT(tuple_stream);
                    }

                    // check and mark for BB similarity
                    for (auto & ele : bBMap) {
                        if (ele.second > 1) {
                            similar_binaries = true;
                            break;
                        }
                    }

                    std::stringstream ending;
                    string success_compilations = "SUCCESS: " + (std::to_string(entry.successful_compilations));
                    string failed_compilations = "FAILED: " + (std::to_string(entry.failed_compilations));
                    LOG(tuple_stream, name);
                    LOG(tuple_stream, similar_binaries);
                    LOG(tuple_stream, success_compilations);
                    LOG(tuple_stream, failed_compilations);
                    LOG_NEXT(tuple_stream);
                    LOG_NEXT(tuple_stream);

                    LOG_END_TUPLE(tuple_stream);

                }

                // ENDING MESSAGE
                if (getenv("RT_LOG")) {
                    log_stream << "Time: " << total_runtime << "s, " << "<Compilation Data> Successful: " << total_successful_compilations << ", Failed:" << total_failed_compilations << std::endl;
                } else {
                    std::cout << "Time: " << total_runtime << "s, " << "<Compilation Data> Successful: " << total_successful_compilations << ", Failed:" << total_failed_compilations << std::endl;
                }

                // add new functions to rtBin
                if(getenv("SAVE_BIN") != NULL) {
                    for(auto & dat : data_interp) {
                        rt_stream << dat << std::endl;
                    }
					rt_stream.close();
                }
                if(getenv("RT_LOG") != NULL) {
					log_stream.close();
				}
			}

			public:
		};

	} // namespace

auto fileLogger = std::make_unique<FileLogger>();

void rtC::createEntry(CallContext & call) {
    if(fileLogger)
        fileLogger->createEntry(call);
}

void rtC::addDispatchInfo(
    std::string & funName,
    size_t & key,
    double & runtime,
    Context & original_intention,
    bool & tragedy,
    bool & deopt
) {
    if(fileLogger)
        fileLogger->addDispatchInfo(funName, key, runtime, original_intention, tragedy, deopt);
}

void rtC::saveCompilationData(
    SEXP callee,
    Context compiled_context,
    double time,
    int bb,
    int p
) {
    if(fileLogger)
        fileLogger->saveCompilationData(callee, compiled_context, time, bb, p);
}

void rtC::registerFailedCompilation(SEXP callee) {
    if(fileLogger)
        fileLogger->registerFailedCompilation(callee);
}

void rtC::saveFunctionMap(std::unordered_map<std::string, bool> map) {
    if(fileLogger)
        fileLogger->saveFunctionMap(map);
}

} // namespace rir
