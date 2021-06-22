#include <iostream>
#include <unordered_map>
#include <vector>
#include "../interpreter/call_context.h"
#include "ContextualProfiling.h"
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

namespace rir {

	namespace {
		using namespace std;

		// Some functions are named, some are anonymous:
		// FunLabel is a base class used to get the name of the function
		// in both cases
		class FunLabel {
			public:
				virtual ~FunLabel() = default;
				virtual string get_name() = 0;
				virtual bool is_anon() = 0;
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

		unordered_map<size_t, unique_ptr<FunLabel>> names; // names: either a string, or an id for anonymous functions
		string del = ","; // delimier


		class ContextDispatchData {
			public:
			int call_count_in_ctxt = 0;
			int successful_compilation_count = 0;
			int failed_compilation_count = 0;
			double time_spent_in_compilation = 0;
			double time_wasted_in_compilation = 0;
			// Count the number of time the version for the context C
			// has been called in this context in
			//     version_called_count[C]
			unordered_map<Context, int> version_called_count;
			unordered_map<Context, int> version_success_run_count;
			unordered_map<Context, double> version_runtime;
			int compile_sequence_number = -1;
			bool multiple_compilation = false;
		};

		class Entry {
			public:
			int total_call_count = 0;
			// per context call and dispatch data
			unordered_map<Context, ContextDispatchData> dispatch_data;
			// needed to disassemble rir code
			SEXP body;
		};

		// Map from a function (identified by its body) to the data about the
		// different contexts it has been called in
		unordered_map<size_t, Entry> entries;

		struct FileLogger {
			ofstream myfile;
			string runId;
			string disassemblyPath;

			FileLogger() {
				// use ISO 8601 date as log name
				time_t timenow = chrono::system_clock::to_time_t(chrono::system_clock::now());
				stringstream runId_ss;
				runId_ss << put_time( localtime( &timenow ), "%FT%T%z" );
				if(getenv("OUT_NAME") != NULL) {
					runId = getenv("OUT_NAME");
				} else {
					runId = runId_ss.str();
				}

				std::cout << "Saving logs in: " << runId;

				myfile.open("profile/" + runId + ".csv");
				myfile << "ID,NAME,CONTEXT,N_CALL,TOTAL_RUNTIME,MULTIPLE_CMP,LAST_SEQUENCE_NO,CMP_SUCCESS,SUCCESS_TIME,CMP_FAIL,FAIL_TIME,GOODNESS,DISPATCHED FUNCTIONS\n";

				const string tmp = "profile/" + runId;
				const char* path = &tmp[0];
				if (mkdir(path, 0777) == -1)
					cerr << "Error :  " << strerror(errno) << endl;
				disassemblyPath = tmp + "/";
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

			void registerFunctionName(CallContext const& call) {
				static const SEXP double_colons = Rf_install("::");
				static const SEXP triple_colons = Rf_install(":::");
				size_t const currentKey = getEntryKey(call.callee);
				SEXP const lhs = CAR(call.ast);

				if (names.count(currentKey) == 0 || names[currentKey]->is_anon() ) {
					if (TYPEOF(lhs) == SYMSXP) {
						// case 1: function call of the form f(x,y,z)
						names[currentKey] = make_unique<FunLabel_named>(CHAR(PRINTNAME(lhs)));
					} else if (TYPEOF(lhs) == LANGSXP && ((CAR(lhs) == double_colons) || (CAR(lhs) == triple_colons))) {
						// case 2: function call of the form pkg::f(x,y,z) or pkg:::f(x,y,z)
						SEXP const fun1 = CAR(lhs);
						SEXP const pkg = CADR(lhs);
						SEXP const fun2 = CADDR(lhs);
						assert(TYPEOF(pkg) == SYMSXP && TYPEOF(fun2) == SYMSXP);
						stringstream ss;
						ss << CHAR(PRINTNAME(pkg)) << CHAR(PRINTNAME(fun1)) << CHAR(PRINTNAME(fun2));
						names[currentKey] = make_unique<FunLabel_named>(ss.str());
					}
				}
				if (names.count(currentKey) == 0) {
					// case 3: function call of the form F()(x, y, z)
					// and this anonymous function has not been seen before
					/*
						TODO: Find a way to recover the name of named functions passed anonymously.
						This mechanism would also handle `::` and `:::`. MWE:

							F <- function() { identity }
							for (i in 1:10) { F()(1) }
					*/
					static int anon_fun_counter = 0;
					names[currentKey] = make_unique<FunLabel_anon>(anon_fun_counter);
					anon_fun_counter++;
				}
			}

			string getFunType(int type) {
				switch(type) {
					case SPECIALSXP: return "SPECIALSXP";
					case BUILTINSXP: return "BUILTINSXP";
					case CLOSXP: return "CLOSXP";
					default:
						stringstream ss;
						ss << "TYPE_NO_" << type;
						return ss.str();
					}
			}

			void createEntry(CallContext const& call) {
				registerFunctionName(call);

				auto fun_id = getEntryKey(call.callee);
				// create or get entry
				auto & entry = entries[fun_id];
				entry.total_call_count++;

				// /!\ do not rely on call.givenContext here, it will
				// be inferred just before the compilation happens

				// TODO CREATE CALL GRAPHS FOR CONTINUING CALL CONTEXTS
			}

			void addFunctionDispatchInfo(
				size_t id,
				Context call_context,
				Function const & f,
				SEXP body
			) {
				Context version_context = f.context();

				// find entry for this function
				// entry must have been previously created by a call to createEntry
				auto & entry = entries.at(id);

				entry.body = body;

				// create or get call context data
				auto & ctxt_data = entry.dispatch_data[call_context];
				ctxt_data.call_count_in_ctxt++;

				// count one call in the context callContextId  to version compiled for funContextId
				ctxt_data.version_called_count[version_context]++;
			}

			void addFunctionDispatchRuntime(
				size_t id,
				Context call_context,
				Function const & f,
				std::chrono::duration<double> duration
			) {
				Context version_context = f.context();

				// find entry for this function
				// entry must have been previously created by a call to createEntry
				auto & entry = entries.at(id);

				// create or get call context data
				auto & ctxt_data = entry.dispatch_data[call_context];

				// count one call in the context callContextId  to version compiled for funContextId

				ctxt_data.version_success_run_count[version_context]++;
				ctxt_data.version_runtime[version_context] += duration.count();
			}

			// For the two functions below: function entry must have been previously
			// created by createEntry, context entry may not exist yet
			void countSuccessfulCompilation(
				SEXP callee,
				Context call_ctxt,
				std::chrono::duration<double> duration,
				pir::ClosureVersion* c,
				Function* fun,
				int sequence
			) {
				size_t entry_key = getEntryKey(callee);

				auto & entry = entries.at(entry_key);
				auto & dispatch_data = entry.dispatch_data[call_ctxt];

				// number of successful compilations and time taken in total
				dispatch_data.successful_compilation_count++;
				dispatch_data.time_spent_in_compilation += duration.count();

				// record compilation sequence number
				if(dispatch_data.compile_sequence_number != -1) {
					dispatch_data.multiple_compilation = true;
				} else {
					dispatch_data.compile_sequence_number = sequence;
				}

				handlePrint(entry_key,call_ctxt,c,false);

			}

			void handlePrint(size_t entry_key, Context call_ctxt, pir::ClosureVersion* c, bool early) {
				static int num = 0;
				num++;
				// store a pointer to the colsure version, makes it easy to print pir
				ofstream fileSteam;
				string filePath = disassemblyPath + std::to_string(entry_key) + "-" + std::to_string(call_ctxt.toI()) + "_" + std::to_string(num);
				fileSteam.open(filePath, std::ios_base::app);
				if(early) {
					fileSteam << "\n\n PIR(EARLY) -- START \n\n";
					fileSteam << "call context: [" << getContextString(call_ctxt) << "]\n";
				} else {
					fileSteam << "\n\n PIR(OPTIMIZED) -- START \n\n";
					fileSteam << "call context: [" << getContextString(call_ctxt) << "]\n";
				}
				c->printStandard(fileSteam, false, false);
				if(early) {
					fileSteam << "\n\n PIR(EARLY) -- END \n\n";
				} else {
					fileSteam << "\n\n PIR(OPTIMIZED) -- END \n\n";
				}
				fileSteam.close();

				string graphPath = disassemblyPath + std::to_string(entry_key) + "-" + std::to_string(call_ctxt.toI()) + "_" + std::to_string(num) + "Graph.DOT";
				ofstream graphStream;
				graphStream.open(graphPath);
				c->printGraph(graphStream, false);

				graphStream.close();

				string graphPathBB = disassemblyPath + std::to_string(entry_key) + "-" + std::to_string(call_ctxt.toI()) + "_" + std::to_string(num) + "BBGraph.DOT";
				ofstream graphStreamBB;
				graphStreamBB.open(graphPathBB);

				c->printBBGraph(graphStreamBB, false);

				graphStreamBB.close();
			}

			void pirBeforeCompilation(
				SEXP callee,
				Context call_ctxt,
				pir::ClosureVersion* c
			) {
				size_t entry_key = getEntryKey(callee);
				handlePrint(entry_key,call_ctxt,c, true);
			}

			bool compileOnlyOnce(
				size_t entry_key,
				Context call_context
			) {
				auto & entry = entries.at(entry_key);
				auto & dispatch_data = entry.dispatch_data[call_context];

				if(dispatch_data.successful_compilation_count > 0 || dispatch_data.failed_compilation_count > 0) {
					return false;
				} else {
					return true;
				}
			}

			void countFailedCompilation(
				SEXP callee,
				Context call_ctxt,
				std::chrono::duration<double> duration,
				int sequence
			) {
				size_t entry_key = getEntryKey(callee);

				auto & entry = entries.at(entry_key);
				auto & dispatch_data = entry.dispatch_data[call_ctxt];

				dispatch_data.failed_compilation_count++;
				dispatch_data.time_wasted_in_compilation += duration.count();

				// record compilation sequence number
				if(dispatch_data.compile_sequence_number != -1) {
					dispatch_data.multiple_compilation = true;
				} else {
					dispatch_data.compile_sequence_number = sequence;
				}
			}

			void disassembleToFile(string& filename, SEXP what, ofstream& out) {
				auto t = DispatchTable::unpack(what);

				if (!t) {
					std::cout << "failed disassembly, no dispatch table\n";
					return;
				}

				out << "== closure " << what << " (dispatch table " << t << ", env "
						<< CLOENV(what) << ") ==\n";
				for (size_t entry = 0; entry < t->size(); ++entry) {
					Function* f = t->get(entry);
					out << "= version " << entry << " (" << f << ") =\n";
					f->disassemble(out);
				}
			}


			std::string getContextString(Context c) {
				stringstream contextString;
				contextString << "<";
				TypeAssumption types[5][6] = {
					{
						TypeAssumption::Arg0IsEager_,
						TypeAssumption::Arg1IsEager_,
						TypeAssumption::Arg2IsEager_,
						TypeAssumption::Arg3IsEager_,
						TypeAssumption::Arg4IsEager_,
						TypeAssumption::Arg5IsEager_,
					},
					{
						TypeAssumption::Arg0IsNonRefl_,
						TypeAssumption::Arg1IsNonRefl_,
						TypeAssumption::Arg2IsNonRefl_,
						TypeAssumption::Arg3IsNonRefl_,
						TypeAssumption::Arg4IsNonRefl_,
						TypeAssumption::Arg5IsNonRefl_,
					},
					{
						TypeAssumption::Arg0IsNotObj_,
						TypeAssumption::Arg1IsNotObj_,
						TypeAssumption::Arg2IsNotObj_,
						TypeAssumption::Arg3IsNotObj_,
						TypeAssumption::Arg4IsNotObj_,
						TypeAssumption::Arg5IsNotObj_,
					},
					{
						TypeAssumption::Arg0IsSimpleInt_,
						TypeAssumption::Arg1IsSimpleInt_,
						TypeAssumption::Arg2IsSimpleInt_,
						TypeAssumption::Arg3IsSimpleInt_,
						TypeAssumption::Arg4IsSimpleInt_,
						TypeAssumption::Arg5IsSimpleInt_,
					},
					{
						TypeAssumption::Arg0IsSimpleReal_,
						TypeAssumption::Arg1IsSimpleReal_,
						TypeAssumption::Arg2IsSimpleReal_,
						TypeAssumption::Arg3IsSimpleReal_,
						TypeAssumption::Arg4IsSimpleReal_,
						TypeAssumption::Arg5IsSimpleReal_,
					}
				};


				// assumptions:
				//    Eager
				//    non reflective
				//    non object
				//    simple Integer
				//    simple Real
				std::vector<char> letters = {'E', 'r', 'o', 'I', 'R'};
				for(int i_arg = 0; i_arg < 6; i_arg++) {
					std::vector<char> arg_str;
				    for(int i_assum = 0; i_assum < 5; i_assum++) {
				        if(c.includes(types[i_assum][i_arg])) {
							arg_str.emplace_back(letters.at(i_assum));
				        }
				    }
					if (! arg_str.empty()) {
						contextString << i_arg << ":";
						for(auto c : arg_str) {
							contextString << c;
						}
						contextString << " ";
					}
				}

				contextString << "|";

				vector<string> assum_strings;
				if(c.includes(Assumption::CorrectOrderOfArguments)) {
					assum_strings.emplace_back("O");
				}

				if(c.includes(Assumption::NoExplicitlyMissingArgs)) {
					assum_strings.emplace_back("mi");
				}

				if(c.includes(Assumption::NotTooManyArguments)) {
					assum_strings.emplace_back("ma");
				}

				if(c.includes(Assumption::StaticallyArgmatched)) {
					assum_strings.emplace_back("Stat");
				}

				if (! assum_strings.empty()) {
					contextString << " ";
				}

				for(size_t i = 0 ; i < assum_strings.size(); i++) {
					contextString << assum_strings[i];
					if (i < assum_strings.size() - 1) {
						contextString << "-";
					}
				}

				contextString << ">";
				return contextString.str();
			}

			void createCodePointEntry(
				int line,
				std::string function,
				std::string name
			) {
				cout << "Line: " << line << ", ";
				cout << "[ " << function << " ]";
				cout << " : " << name << " ";
				cout << "\n";
			}

			~FileLogger() {
				double total_time_in_compilation = 0;
				for (auto const & ir : entries) {
					auto fun_id = ir.first;
					auto & entry = ir.second;
					string name = names.at(fun_id)->get_name(); // function name

					double baselineAvgRuntime = 0;
					double baselineTriggered = false;

					// iterate over contexts
					for (auto const & itr : entry.dispatch_data) {
						// *itr -> Context
						auto call_ctxt = itr.first; // current context __id
						auto & dispatch_data = itr.second; // current context

						string currContextString = getContextString(call_ctxt); // current context __string

						stringstream contextsDispatched;

						double totalRuntimeUnderContext = 0;

						double goodness = false;

						int otherRuntimeCount = 0;
						double otherAvgRuntime = 0;

						bool multiple_compilation = dispatch_data.multiple_compilation;
						int last_sequence_number = dispatch_data.compile_sequence_number;

						// iterate over dispatched functions under this context
						for (auto const & itr1 : dispatch_data.version_called_count) {
							// *itr1 -> Context
							Context version_context = itr1.first; // current function context
							int functionContextCallCount = itr1.second; // current function context __call count
							string currContextString = getContextString(version_context); // current function context __string

							// Is this baseline ?
							bool baseline = version_context.toI() == 0 ? true : false;


							// number of times the functions ran successfully
							unordered_map<Context, int> version_success_run_count = dispatch_data.version_success_run_count;
							double success_runs = version_success_run_count[version_context];
							// total runtime of all the runs successful runs
							unordered_map<Context, double> version_runtime = dispatch_data.version_runtime;
							double success_runtime = version_runtime[version_context];

							// average time to run the dispatched function
							double avg_runtime = 0;
							totalRuntimeUnderContext += success_runtime;

							// error ?
							bool error = false;

							// avg runtime
							if(success_runs > 0) {
								avg_runtime = success_runtime / success_runs;
							}

							if(baseline) {
								baselineAvgRuntime = avg_runtime;
								baselineTriggered=true;
							} else if(success_runs > 0) {
								otherRuntimeCount++;
								otherAvgRuntime += avg_runtime;
							} else { // dispatched function failed to run successfully even once
								error=true;
							}

							contextsDispatched
								<< "["
								<< functionContextCallCount
								<< "]{"
								<< avg_runtime
								<< "}"
								<< (error ? "(ERR)" : "")
								<< currContextString
								<< " ";
						}
						otherAvgRuntime = otherAvgRuntime / otherRuntimeCount;
						total_time_in_compilation += dispatch_data.time_spent_in_compilation;
						if(otherAvgRuntime < baselineAvgRuntime) {
							goodness = true;
						}

						if(!goodness) {
							// disassemble and log the bad functions
							// ofstream fileSteam;
							string filePath = disassemblyPath + std::to_string(fun_id) + "-" + std::to_string(call_ctxt.toI());
							// fileSteam.open(filePath, std::ios_base::app);

							// disassembleToFile(filePath, entry.body, fileSteam);
							// fileSteam.close();
							rename( &filePath[0] , &(filePath + "-BAD")[0] );
						}

						// print row
						myfile
							<< fun_id // id
							<< del
							<< name // name
							<< del
							<< currContextString // call context
							<< del
							<< dispatch_data.call_count_in_ctxt // call context count
							<< del
							<< totalRuntimeUnderContext // total runtime under context
							<< del
							<< multiple_compilation
							<< del
							<< last_sequence_number
							<< del
							<< dispatch_data.successful_compilation_count // number of successful compilations in this context
							<< del
							<< dispatch_data.time_spent_in_compilation // time spent for successful compilation (all the compilations under a call context)
							<< del
							<< dispatch_data.failed_compilation_count // number of failed compilations in this context
							<< del
							<< dispatch_data.time_wasted_in_compilation // time wasted trying to compile
							<< del
							<< (!baselineTriggered ? "NA(NO BASELINE)" : goodness ?
								"TRUE(Speedup: " + std::to_string((((baselineAvgRuntime/otherAvgRuntime)-1)*100)) + "%)" :
								dispatch_data.version_called_count.size() == 1 ? "NA" :
									"FALSE("+disassemblyPath + std::to_string(fun_id) + "-" + std::to_string(call_ctxt.toI())+"" )
							<< del
							<< contextsDispatched.str() // functions dispatched under this context
							<< "\n";
					}
				}
				myfile
					<< del
					<< del
					<< del
					<< del
					<< del
					<< del
					<< total_time_in_compilation // Total time in compilation
					<< del
					<< del
					<< del
					<< "\n";
				myfile.close();
			}

			public:
		};

	} // namespace

auto fileLogger = getenv("CONTEXT_LOGS") ? std::make_unique<FileLogger>() : nullptr;

void ContextualProfiling::createCallEntry(
		CallContext const& call) {
	if(fileLogger) {
		fileLogger->createEntry(call);
	}
}

void ContextualProfiling::recordCodePoint(
		int line,
		std::string function,
		std::string name
		) {
	if(fileLogger) {
		fileLogger->createCodePointEntry(line, function, name);
	}
}

size_t ContextualProfiling::getEntryKey(CallContext const& cc) {
	if(fileLogger) {
		return fileLogger->getEntryKey(cc.callee);
	} else {
		return 0;
	}
}

void ContextualProfiling::addFunctionDispatchInfo(
	size_t id,
	Context contextCaller,
	Function const &f,
	SEXP body
) {
	if(fileLogger) {
		return fileLogger->addFunctionDispatchInfo(id, contextCaller, f, body);
	}
}


void ContextualProfiling::countSuccessfulCompilation(
	SEXP callee,
	Context assumptions,
	std::chrono::duration<double> duration,
	pir::ClosureVersion* c,
	Function* fun,
	int sequence
) {
	if (fileLogger) {
		fileLogger->countSuccessfulCompilation(callee, assumptions, duration, c, fun, sequence);
	}
}

void ContextualProfiling::countFailedCompilation(
	SEXP callee,
	Context assumptions,
	std::chrono::duration<double> duration,
	int sequence
) {
	if (fileLogger) {
		fileLogger->countFailedCompilation(callee, assumptions, duration, sequence);
	}
}

bool ContextualProfiling::compileFlag(
	size_t id,
	Context contextCaller
) {
	if (getenv("SKIP_ALL_COMPILATION")) {
		return false;
	}
	if (!getenv("COMPILE_ONLY_ONCE")) {
		return true;
	}
	if (fileLogger) {
		return fileLogger->compileOnlyOnce(id, contextCaller);
	} else {
		return true;
	}
}

void ContextualProfiling::addFunctionDispatchRuntime(
	size_t id,
	Context contextCaller,
	Function const &f,
	std::chrono::duration<double> duration

) {
	if(fileLogger) {
		return fileLogger->addFunctionDispatchRuntime(id, contextCaller, f, duration);
	}
}

void ContextualProfiling::pirBeforeCompilation(
	SEXP callee,
	Context assumptions,
	pir::ClosureVersion* c
) {
	if (fileLogger) {
		fileLogger->pirBeforeCompilation(callee, assumptions, c);
	}
}


} // namespace rir
