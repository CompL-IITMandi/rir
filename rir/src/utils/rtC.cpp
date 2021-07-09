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

		class FunIter {
			private:
			public:
				double version_called_count = 0;
				double version_success_run_count = 0;
				double version_runtime = 0;
				double version_deopt_time = 0;
				vector<Context> calling_context;

				FunIter(double a, double b, double c, double d, Context con) {
					add(a,b,c,d,con);
				}

				void add(double a, double b, double c,double d,Context call_context) {
					version_called_count += a;
					version_success_run_count += b;
					version_runtime += c;
					version_deopt_time += d;
					calling_context.push_back(call_context);
				}
		};

		// Some functions are named, some are anonymous:
		// FunLabel is a base class used to get the name of the function
		// in both cases
		class FunLabel {
			public:
				virtual ~FunLabel() = default;
				virtual string get_name() = 0;
				virtual bool is_anon() = 0;
		};


		class FunLabel_named : public FunLabel {
		private:
			string const name;
		public:
			FunLabel_named(string name) : name{move(name)} {};
			string get_name() override {
				return name;
			}
			bool is_anon() override { return false; }
		};

        class FunLabel_anon : public FunLabel {
		private:
			int const id = 0;
		public:
			FunLabel_anon(int id) : id{id} {};
			string get_name() override {
				stringstream ss;
				ss << "*ANON_FUN_" << id << "*";
				return ss.str();
			}
			bool is_anon() override { return true; }
		};

		// unordered_map<size_t, unique_ptr<FunLabel>> names; // names: either a string, or an id for anonymous functions
        unordered_map<size_t, std::string> names;
		string del = ","; // delimiter

        static int OPTIMISM = 10;
        static int FAIRNESS = 10;

		class DispatchData {
			public:
                double call_count = 0;
                double success_run_count = 0;

                double compilation_time = 0;

                double run_time = 0;

                double bb_count = 0;
                double p_count = 0;

                double forced_count = 0;
                double forced_runtime = 0;

                double averageForcedRuntime() {
                    return forced_runtime/forced_count;
                }

                double averageRuntime() {
                    return run_time/success_run_count;
                }

                double successRate() {
                    return (success_run_count/call_count)*100;
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

                double total_call_count = 0;

                double baseline_run_time = 0;
                double baseline_call_count = 0;

                double averageBaselineRuntime() {
                    return baseline_run_time/baseline_call_count;
                }

                bool isGoodForJit(Context c, int under) {
                    auto & data = dispatch_data_holder[c];
                    return (
                        ((data.averageRuntime()*under) + data.compilation_time) < (averageBaselineRuntime()*under)
                    );
                }

                bool isGoodForAot(Context c) {
                    auto & data = dispatch_data_holder[c];
                    return (
                        data.averageRuntime() < averageBaselineRuntime()
                    );
                }
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
			FileLogger() {

                if(getenv("RT_LOG") != NULL) {
					string binId = getenv("SAVE_BIN");
					log_stream.open(binId + ".rtLog");
				}
			}
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
                std::string name,
                size_t id,
                Context dispatched_context,
                bool baseline,
                bool deopt,
                double runtime,
                bool tragedy,
                Context original_intention
			) {
                names[id] = name;
				// create or get entry
                auto & entry = entries[id];

                total_runtime += runtime;

                if(tragedy) { // we are forcing a context to run baseline forcefully
                    entry.dispatch_data_holder[original_intention].forced_count++;
                    entry.dispatch_data_holder[original_intention].forced_runtime += runtime;
                }

                if(baseline) {
                    entry.baseline_call_count++;
                    entry.baseline_run_time += runtime;
                    return;
                }

                // did the function run successfully or did it deoptimize ???
                if (!deopt) {
                    entry.dispatch_data_holder[dispatched_context].call_count++;
                    entry.dispatch_data_holder[dispatched_context].run_time += runtime;
                    entry.dispatch_data_holder[dispatched_context].success_run_count++;
                } else {
                    entry.dispatch_data_holder[dispatched_context].call_count++;
                    entry.dispatch_data_holder[dispatched_context].run_time += runtime;
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

                entry.dispatch_data_holder[compiled_context].compilation_time += time;
                entry.dispatch_data_holder[compiled_context].bb_count = bb;
                entry.dispatch_data_holder[compiled_context].p_count = p;
            }

            void registerFailedCompilation() {
                total_failed_compilations++;
            }

            void saveFunctionMap(std::unordered_map<std::string, bool> map) {
                function_map = map;
            }

			~FileLogger() {
                if(getenv("SAVE_BIN") != NULL) {
                    std::cout << "saving BIN" << std::endl;
					string binId = getenv("SAVE_BIN");
					rt_stream.open(binId + ".rtBin");
				}
                for (auto & itr : function_map) {
                    string key = itr.first;
                    if(getenv("SAVE_BIN") != NULL)
                    rt_stream << key << std::endl;
                }
                set<string> data_interp;
                for (auto & itr : entries) {
                    string name = names[itr.first];
                    Entry & entry = itr.second;

                    if (name.compare("") == 0) continue; // skip over anonymous functions
                    if (entry.dispatch_data_holder.size() == 0) continue; // skip over functions with no specialization

                    double baseline_avg = entry.averageBaselineRuntime();

                    stringstream ss;

                    if (getenv("RT_LOG")) {
                        ss
                            << std::left
                            << name
                            << "[" << entry.total_call_count << "]"
                            << ": {baseline_avg ="
                            << baseline_avg
                            << "} ";
                    }

                    for (auto & con : entry.dispatch_data_holder) {
                        auto & dispatched = con.second;
                        // not enough data to mark bad
                        if(!dispatched.calculable()) {
                            if (getenv("RT_LOG")) {
                                ss
                                    << "("
                                    << dispatched.call_count << ","
                                    << dispatched.forced_count << ","
                                    << dispatched.success_run_count
                                    << ")"
                                    << "{"
                                    << ((Context)con.first).toI()
                                    << ": Not Enough Data}";
                            }
                            continue;
                        }

                        double dispatched_avg = dispatched.averageRuntime();

                        if (getenv("RT_LOG")) {
                            ss
                                << "("
                                << dispatched.call_count << ","
                                << dispatched.forced_count << ","
                                << dispatched.success_run_count
                                << ")"
                                << "[";
                            if(dispatched.jitBreakEvenPoint() > 0) {
                                ss << " JIT" << dispatched.jitBreakEvenPoint() << " ";
                                ss << fixed << setprecision(1) << dispatched.speedup() << "X";
                            }
                            ss
                                << " ]"
                                << "{"
                                << ((Context)con.first).toI()
                                << ": "
                                << dispatched_avg << "(" << dispatched.successRate() << ")"
                                << "} ";
                        }

                        if(getenv("SAVE_BIN") != NULL) {
                            // if an entry is not good for AOT
                            if(!dispatched.isGoodForAot())
                                // skip if the entry is already accounted for
                                if(function_map.find(name + std::to_string(((Context)con.first).toI())) == function_map.end())
                                    data_interp.insert((name + std::to_string(((Context)con.first).toI())));
                            // If the JIT breakeven is greater than twice the call count
                            // if(dispatched.jitBreakEvenPoint() > ((dispatched.call_count + dispatched.forced_count)*2)) {
                            //     if(function_map.find(name + std::to_string(((Context)con.first).toI())) == function_map.end())
                            //         data_interp.insert((name + std::to_string(((Context)con.first).toI())));
                            // }
                        }

                    }

                    if (getenv("RT_LOG")) {
                        ss << std::endl;
                        log_stream << ss.str();
                    } else {
                        std::cout << ss.str();
                    }

                }

                if (getenv("RT_LOG")) {
                    log_stream << "Time: " << total_runtime << "s, " << "<Compilation Data> Successful: " << total_successful_compilations << ", Failed:" << total_failed_compilations << std::endl;
                } else {
                    std::cout << "Time: " << total_runtime << "s, " << "<Compilation Data> Successful: " << total_successful_compilations << ", Failed:" << total_failed_compilations << std::endl;
                }
                if(getenv("SAVE_BIN") != NULL) {
                    for(auto & dat : data_interp) {
                        rt_stream << dat << std::endl;
                    }
                }
                if(getenv("SAVE_BIN") != NULL) {
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
    std::string name,
    size_t id,
    Context dispatched_context,
    bool baseline,
    bool deopt,
    double runtime,
    bool tragedy,
    Context original_intention
) {
    if(fileLogger)
        fileLogger->addDispatchInfo(name, id, dispatched_context, baseline, deopt, runtime, tragedy, original_intention);
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

void rtC::registerFailedCompilation() {
    if(fileLogger)
        fileLogger->registerFailedCompilation();
}

void rtC::saveFunctionMap(std::unordered_map<std::string, bool> map) {
    if(fileLogger)
        fileLogger->saveFunctionMap(map);
}

} // namespace rir
