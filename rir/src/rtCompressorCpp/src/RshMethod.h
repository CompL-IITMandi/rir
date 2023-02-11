#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
extern double R_totalRuntime;

struct RshMethod {
  std::string id, name, context, hast;
  bool compiled = false;
  unsigned long contextI = 0;
  double rir2pir=0.0, opt = 0.0, bb = 0.0, p = 0.0, failed = 0.0, runtime = 0.0, effect = 0.0;

  std::set<std::string> callsTo;

  void writeToStream(std::ofstream & file) {
    file << "[";
    file << "\"" << id      << "\"" << ",";;
    file << "\"" << name    << "\"" << ",";;
    file << "\"" << context << "\"" << ",";;

    file << ((runtime / R_totalRuntime) * 100)  << ",";
    file << rir2pir << ",";
    file << opt     << ",";
    file << bb      << ",";
    file << p;

    file << "]";
  }
};

const std::vector<std::string> RshMethodParams {
  "id", "name", "context", "hast",
  "compiled",
  "contextI",
  "rir2pir", "opt", "bb", ""
};

struct ContextRuntimeSummary {
  int runs = 0;
  unsigned long contextI = 0;
  double cmp = 0.0, opt = 0.0, run = 0.0;
};

struct MethodRuntimeSummary {
  std::string hast;
  double rir2pir = 0.0, opt = 0.0, runtime = 0.0, effect = 0.0;
};

template <typename Parameter>
struct TempSortObj {
  std::string id;
  Parameter param;
};

template <typename Parameter>
struct ObjectSorter {
  inline bool operator() (const TempSortObj<Parameter>& struct1, const TempSortObj<Parameter>& struct2) {
    return (struct1.param > struct2.param);
  }
};

struct Meta {
  std::string name;
  int runs;
};

extern void (*singleLineObjectSerializer)(RshMethod &,std::ofstream &,std::string &);

extern void(*lineToObjectParser)(RshMethod &,std::string &,std::string &);

std::ostream& operator<<(std::ostream& os, const RshMethod& m);
