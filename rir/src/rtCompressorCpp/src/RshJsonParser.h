#pragma once
#include <iostream>
#include <string>
#include <stack>
#include <vector>
#include <sstream>
#include <fstream>
#include "./HugeStackManager.h"
#include "./RshMethod.h"
#include <stdlib.h>
#include <time.h>
#include <unordered_map>
#include <set>
#include <algorithm>

#include "Utility.h"
#include "JsonWriter.h"
#define BLACKLIST_THRESHOLD 10

class RshJsonParser {
  private:
  HugeStackManager<RshMethod>* hsm;
  std::string & del;

  bool verbose = false;
  int blacklisted = 0, whitelisted = 0, failed = 0;

  std::unordered_map<std::string, std::unordered_map<std::string,std::vector<double>>> methodCSMap;     // [id,context] -> [runtimes]
  std::unordered_map<std::string, std::unordered_map<std::string,ContextRuntimeSummary>> methodCTMap;   // [id,context] -> { runs, cmp, opt, run }
  std::unordered_map<std::string, MethodRuntimeSummary> methodRuntimeMap;                               // id -> { rir2pir, opt, runtime, effect; }
  std::unordered_map<std::string, std::set<std::string>> methodContextMap;                              // id -> set [ context ]
  std::unordered_map<std::string, std::set<std::string>> methodCallMap;                                 // id -> set [ ids ]
  std::unordered_map<std::string, std::string> methodNameMap;                                           // id -> name
  std::vector<RshMethod> methodVersionCallMap;                                                          // vector [ RshMethod ]

  std::unordered_map<std::string, Meta> methodMeta;                                                     // id -> { name, runs }
  std::unordered_map<std::string, std::vector<unsigned long>> blacklistMap;                             // id -> { name, runs }
  std::unordered_map<std::string, std::vector<unsigned long>> whitelistMap;                             // id -> { name, runs }

  std::unordered_map<std::string, std::vector<int>> boxContextCallRuns;                                 // id -> [ contexts ]
  int colorGenerator = 1;
  std::unordered_map<std::string, int> boxContextResolution;                                            // context -> uid

  std::unordered_map<std::string, std::vector<RshMethod>> methodsThatCompiledMap;                       // id -> [ RshMethod ]

  std::vector<std::string> s_runtime, s_calls, s_contexts;                                              // [ ids ], [ ids ], [ ids ]

  double totalRuntime;

  void pushMethod(std::string & line) {
    RshMethod method;
    auto callback = [&](const int & index, std::string & token) {
      switch(index) {
        case 1: method.id   = removeQuotes(token); break;
        case 2: method.hast = token              ; break;
        case 3: method.name = removeQuotes(token); break;
      }
    };
    handleLine(line, ",", callback);
    methodNameMap[method.id] = method.name;
    if (!hsm->empty()) {
      RshMethod parent = hsm->pop();
      updateMethodCallMap(parent.id, method.id);
      hsm->push(parent);
    }
    hsm->push(method);
  }

  void updateMethodCallMap(std::string & id, std::string & child) {
    methodCallMap[id].insert(child);
  }

  void addMeta(std::string & line) {
    RshMethod method = hsm->pop();
    bool compiled = false;
    double rir2pir, opt;
    int bb, p;
    auto callback = [&](const int & index, std::string & token) {
      switch(index) {
        case 1: if (token.compare("true") == 0) compiled = true; break;
        case 2: rir2pir = std::stod(token); break;
        case 3: opt     = std::stod(token); break;
        case 4: bb      = std::stoi(token); break;
        case 5: p       = std::stoi(token); break;
        case 6:
          if (compiled) {
            while (line.compare(method.id) != 0) {
              failed++;
              method = hsm->pop();
            }
            method.rir2pir = rir2pir;
            method.opt = opt;
            method.bb = bb;
            method.p = p;
          } else {
            method.failed = std::stod(token);
          }
          method.compiled = compiled;
          break;
      }
    };
    handleLine(line, ",", callback);
    hsm->push(method);
  }

  void finalizeMethod(std::string & line) {
    RshMethod method = hsm->pop();
    std::string context, method_id;
    unsigned long contextI = 0;
    double runtime = 0;

    auto callback = [&](const int & index, std::string & token) {
      switch(index) {
        case 1: context=(token.compare("<empty Context>") == 0) ? "baseline" : token; break;
        case 2: line.erase(0, 1); break;
      }
    };
    handleLine(line, "\"", callback);

    auto callback1 = [&](const int & index, std::string & token) {
      switch(index) {
        case 0: contextI = std::stoul(token); break;
        case 1: runtime = std::stod(token); break;
        case 2: method_id = line; break;
      }
    };

    handleLine(line, ",", callback1);

    while (method_id.compare(method.id) != 0) {
      failed++;
      method = hsm->pop();
    }
    if (!hsm->empty()) {
      RshMethod parent = hsm->pop();
      auto id = method.name + "_" + std::to_string(contextI);
      parent.callsTo.insert(id);
      hsm->push(parent);
    }
    method.runtime = runtime;
    method.context = context;
    method.contextI = contextI;
    methodVersionCallMap.push_back(method);
    mapUpdater(method);
  }

  void mapUpdater(RshMethod & method) {
    updateMethodCSMap(method);
    updateMethodCTMap(method);
    updateMethodRuntimeMap(method);
    updateMethodContextMap(method);
    updateMethodMeta(method);
    updateBoxContextCallRuns(method);
    updateMethodsThatCompiledMap(method);
  }

  void updateMethodCSMap(RshMethod & method) {
    methodCSMap[method.id][method.context].push_back(method.runtime);
  }

  void updateMethodCTMap(RshMethod & method) {
    methodCTMap[method.id][method.context].runs++;
    methodCTMap[method.id][method.context].cmp += method.rir2pir;
    methodCTMap[method.id][method.context].opt += method.opt;
    methodCTMap[method.id][method.context].run += method.runtime;
    methodCTMap[method.id][method.context].contextI = method.contextI;
  }

  void updateMethodRuntimeMap(RshMethod & method) {
    methodRuntimeMap[method.id].rir2pir += method.rir2pir;
    methodRuntimeMap[method.id].opt += method.opt;
    methodRuntimeMap[method.id].runtime += method.runtime;
    methodRuntimeMap[method.id].effect += method.effect;
    methodRuntimeMap[method.id].hast = method.hast;
  }

  void updateMethodContextMap(RshMethod & method) {
    methodContextMap[method.id].insert(method.context);
  }

  void updateMethodMeta(RshMethod & method) {
    methodMeta[method.id].name = method.name;
    methodMeta[method.id].runs++;
  }

  void updateBoxContextCallRuns(RshMethod & method) {
    if (boxContextResolution.find(method.context) == boxContextResolution.end()) { // if key does not exist, generate a new one
      int color = colorGenerator++;
      boxContextResolution[method.context] = color;
    }
    boxContextCallRuns[method.id].push_back(boxContextResolution[method.context]);
  }

  void updateMethodsThatCompiledMap(RshMethod & method) {
    if (method.compiled) {
      methodsThatCompiledMap[method.id].push_back(method);
    }
  }

  void createSorts() {
    std::vector<TempSortObj<double>> t_runtime;
    std::vector<TempSortObj<int>> t_contexts, t_calls;
    for (auto & method : methodRuntimeMap) {
      TempSortObj<double> t1;
      t1.id = method.first;
      t1.param = method.second.runtime;
      t_runtime.push_back(t1);

      TempSortObj<int> t2;
      t2.id = method.first;
      t2.param = methodContextMap[method.first].size();
      t_contexts.push_back(t2);

      TempSortObj<int> t3;
      t3.id = method.first;
      t3.param = methodMeta[method.first].runs;
      t_calls.push_back(t3);
    }
    std::sort(t_runtime.begin(), t_runtime.end(), ObjectSorter<double>());
    std::sort(t_contexts.begin(), t_contexts.end(), ObjectSorter<int>());
    std::sort(t_calls.begin(), t_calls.end(), ObjectSorter<int>());
    for (auto & ele : t_runtime) {
      s_runtime.push_back(ele.id);
    }
    for (auto & ele : t_contexts) {
      s_contexts.push_back(ele.id);
    }
    for (auto & ele : t_calls) {
      s_calls.push_back(ele.id);
    }
  }

  void blacklist_fun(std::string & id, std::vector<TempSortObj<int>> & t_runs, int & total_runs, std::vector<unsigned long> & blacklist) {
    if (t_runs.size() <= 1) return;

    TempSortObj<int> last_element = *(--t_runs.end());
    if ((((double)last_element.param / (double)total_runs)*100) > BLACKLIST_THRESHOLD) {
      return;
    } else {
      blacklisted++;
      t_runs.pop_back();
      blacklist.push_back(methodCTMap[id][last_element.id].contextI);
      total_runs -= last_element.param;
      blacklist_fun(id, t_runs, total_runs, blacklist);
    }
  }

  void createContextBlacklist(std::string & id, std::set<std::string> contexts) {
    // remove baseline
    contexts.erase("baseline");
    std::vector<TempSortObj<int>> t_runs;
    std::vector<unsigned long> blacklist;
    std::vector<unsigned long> whitelist;
    int total_runs = 0;
    for (auto & c : contexts) {
      TempSortObj<int> t1;
      t1.id = c;
      t1.param = methodCTMap[id][c].runs;
      total_runs += methodCTMap[id][c].runs;
      t_runs.push_back(t1);
    }
    std::sort(t_runs.begin(), t_runs.end(), ObjectSorter<int>());

    if (verbose) {
      std::cout << std::endl << "method    : ";
      for (auto & item : t_runs) {
        std::cout << methodCTMap[id][item.id].runs << " runs { context: " << methodCTMap[id][item.id].contextI << " } ";
      }
    }

    blacklist_fun(id, t_runs, total_runs, blacklist);
    if (blacklist.size() > 0) blacklistMap[id] = blacklist;

    if (verbose) {
      std::cout << std::endl << "blacklist : ";
      for (auto & item : blacklist) {
        std::cout << item << ",";
      }
      std::cout << std::endl << "whitelist : ";
      for (auto & item : t_runs) {
        whitelisted++;
        whitelist.push_back(methodCTMap[id][item.id].contextI);
        std::cout << methodCTMap[id][item.id].contextI << ",";
      }
      std::cout << std::endl;
    }

    if (whitelist.size() > 0)
    whitelistMap[id] = whitelist;
  }

  public:
  RshJsonParser(std::string & d) : del(d) {
    hsm = new HugeStackManager<RshMethod>(lineToObjectParser,singleLineObjectSerializer,d);
    totalRuntime = 0;
  }

  RshJsonParser(std::string & d, bool verbose) : del(d), verbose(verbose) {
    hsm = new HugeStackManager<RshMethod>(lineToObjectParser,singleLineObjectSerializer,d);
    totalRuntime = 0;
  }

  ~RshJsonParser() {
    std::cout << "Stack Matching Errors: " << failed << ", Whitelisted: " << whitelisted << ", Blacklisted: " << blacklisted << std::endl;
  }

  std::set<std::string> getContext(size_t fid){
    std::stringstream ss;
    ss << fid;
    std::string s = ss.str();
    return methodContextMap[s];
  }

  int getName(size_t fid){
    return methodMeta[std::to_string(fid)].runs;
  }

  std::string getMethodName(size_t fid){
    return methodNameMap[std::to_string(fid)];
  }

  void processLine(std::string & line) {
    int start = 0;
    int end = line.find(del);
    std::string lookahead = line.substr(start, end - start);

    if (lookahead.compare("=") == 0) {
      pushMethod(line);
    } else if (lookahead.compare("!") == 0) {
      finalizeMethod(line);
    } else if (lookahead.compare("@") == 0) {
      addMeta(line);
    } else {
      createSorts();

      size_t pos = 0;
      std::string token;
      while ((pos = line.find(del)) != std::string::npos) {
        token = line.substr(0, pos);
        line.erase(0, pos + del.length());
      }
      totalRuntime = std::stod(line);

      std::cout << "initial parsing completed" << std::endl;

      for (auto & m : methodContextMap) {
        auto method = m.first;
        auto contextSet = m.second;
        if (contextSet.size() > 1) {
          createContextBlacklist(method, contextSet);
        }
      }
    }
  }

  void saveVizJson(std::string path) {
    JsonWriter loggFile(path, false);
    loggFile.addTotalRuntime(totalRuntime);

    loggFile.start();
    loggFile.saveSort("s_runtime", s_runtime, false);
    loggFile.saveSort("s_calls", s_calls, false);
    loggFile.saveSort("s_contexts", s_contexts, false);
    loggFile.saveMethodCTMap("methodCTMap", methodCTMap, false);
    loggFile.saveMethodCSMap("methodCSMap", methodCSMap, false);
    loggFile.saveMethodMeta("methodMeta", methodMeta, false);
    loggFile.saveMethodsThatCompiledMap("methodsThatCompiledMap", methodsThatCompiledMap, false);
    loggFile.saveMethodRuntimeMap("methodRuntimeMap", methodRuntimeMap, false);
    loggFile.saveMethodContextMap("methodContextMap", methodContextMap, false);
    loggFile.saveMethodCallMap("methodCallMap", methodCallMap, false);
    loggFile.saveBoxContextCallRuns("boxContextCallRuns",boxContextCallRuns,false);
    loggFile.saveBoxContextResolution("boxContextResolution", boxContextResolution, false);

    loggFile.addKeyValue("totalRuntime", totalRuntime, true);
    loggFile.end();
  }

  void saveLists(std::string path) {
    std::string binPath = path.substr(0, path.size() - 5);
    std::ofstream whitelistFile, blacklistFile;
    whitelistFile.open(binPath + ".whitelist");
    blacklistFile.open(binPath + ".blacklist");
    for (auto & ele : blacklistMap) {
      blacklistFile << methodRuntimeMap[ele.first].hast << ",";
      for (auto con : ele.second) {
        blacklistFile << con << ",";
      }
      blacklistFile << std::endl;
    }

    for (auto & ele : whitelistMap) {
      whitelistFile << methodRuntimeMap[ele.first].hast << ",";
      for (auto con : ele.second) {
        whitelistFile << con << ",";
      }
      whitelistFile << std::endl;
    }

    blacklistFile.close();
    whitelistFile.close();
  }

  void saveCG(std::string path) {
    std::string binPath = path.substr(0, path.size() - 5);
    std::ofstream callgraph, callgraph_versions;
    callgraph.open(binPath + "_CG.DOT");
    callgraph_versions.open(binPath + "_CGV.DOT");

    callgraph << "digraph {\n";
    callgraph << "rankdir=BT\n";

    std::set<std::string> cg_entries;

    for (auto & id: methodCallMap) {
      auto from = id.first;
      for (auto & to: id.second) {
        std::stringstream ss;
        ss << "\"" << methodMeta[from].name <<  "\"";
        ss << " -> ";
        ss << "\"" << methodMeta[to].name <<  "\"";
        ss << ";\n";
        cg_entries.insert(ss.str());
      }
    }
    for (auto & item : cg_entries) {
      callgraph << item;
    }
    callgraph << "\n}";


    callgraph_versions << "digraph {\n";
    callgraph_versions << "rankdir=BT\n";

    std::set<std::string> cgv_entries;
    for (auto & method : methodVersionCallMap) {
      for (auto & callsToItem : method.callsTo) {
        std::stringstream ss;
        ss << "\"" << method.name << "_" << method.contextI <<  "\"";
        ss << " -> ";
        ss << "\"" << callsToItem <<  "\"";
        ss << ";\n";
        cgv_entries.insert(ss.str());
      }
    }
    for (auto & item : cgv_entries) {
      callgraph_versions << item;
    }
    // { rank=same; b, c, d }

    std::unordered_map<std::string,std::set<std::string>> rank_map;
    for (auto & item : methodContextMap) {
      auto & method_id = item.first;
      auto & contexts = item.second;
      //int index = 0;
      for (auto & version : contexts) {
        std::stringstream ss;
        ss << "\"" << methodMeta[method_id].name << "_" << methodCTMap[method_id][version].contextI << "\"";
        rank_map[method_id].insert(ss.str());
      }
    }
    for (auto & item : rank_map) {
      if (item.second.size() < 2) continue;
      callgraph_versions << "{ rank=same;";
      int index = 0;
      for (auto & cg : item.second) {
        callgraph_versions << " " << cg;
        index++;
        if (index != (int)item.second.size()) callgraph_versions << ",";
      }
      callgraph_versions << " }\n";
    }

    callgraph_versions << "\n}";
    callgraph.close();
    callgraph_versions.close();
  }

  std::vector<std::string> saveCG(){

    std::vector<std::string> callgraph;
    callgraph.push_back("digraph {");
    callgraph.push_back("rankdir=BT");

    std::set<std::string> cg_entries;

    for (auto & id: methodCallMap) {
      auto from = id.first;
      for (auto & to: id.second) {
        std::stringstream ss;
        ss << "\\\"" << methodNameMap[from] <<  "\\\"";
        ss << " -> ";
        ss << "\\\"" << methodNameMap[to] <<  "\\\"";
        ss << ";";
        cg_entries.insert(ss.str());
      }
    }
    for (auto & item : cg_entries) {
      callgraph.push_back(item);
    }
    callgraph.push_back("}");

    return callgraph;

  }

};
