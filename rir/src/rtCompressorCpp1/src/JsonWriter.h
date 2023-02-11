#pragma once
#include <fstream>
#include <string>
#include <unordered_map>
#include "RshMethod.h"
class JsonWriter {

  std::ofstream file;
  bool newLine;

  template <typename T>
  void addKey(const T & key) {
    file << "\"" << key << "\":";
  }

  template <typename T>
  void addValue(const T & value) {
    file << value;
  }

  template <typename T>
  void addVector(const T & vec, bool wrap) {
    startArray();
    int index = 0;
    for (auto & data : vec) {
      if (wrap) {
        file << "\"" << data << "\"";
      } else {
        file << data;
      }
      index++;
      if (index != (int)vec.size()) file << ",";
    }
    endArray();
  }

  void addVector(std::vector<RshMethod> & vec) {
    startArray();
    int index = 0;
    for (auto & item : vec) {
      item.writeToStream(file);
      index++;
      if (index != (int)vec.size()) file << ",";
    }
    endArray();
  }

  void startObject() {
    file << "{";
  }

  void endObject() {
    file << "}";
  }

  void startArray() {
    file << "[";
  }

  void endArray() {
    file << "]";
  }

  void saveObject(std::unordered_map<std::string, int> & values, bool wrap) {
    startObject();
    int index = 0;
    for (auto & pair : values) {
      addKey(pair.first);
      addValue(pair.second);
      index++;
      if (index != (int)values.size()) file << ",";
    }
    endObject();
  }

  void saveObject(std::unordered_map<std::string, std::vector<int>> & values, bool wrap) {
    startObject();
    int index = 0;
    for (auto & pair : values) {
      addKey(pair.first);
      addVector(pair.second, wrap);
      index++;
      if (index != (int)values.size()) file << ",";
    }
    endObject();
  }

  void saveObject(std::unordered_map<std::string, std::vector<double>> & values) {
    startObject();
    int index = 0;
    for (auto & pair : values) {
      addKey(pair.first);
      addVector(pair.second, false);
      index++;
      if (index != (int)values.size()) file << ",";
    }
    endObject();
  }

  void saveObject(std::unordered_map<std::string, std::set<std::string>> & values, bool wrap) {
    startObject();
    int index = 0;
    for (auto & pair : values) {
      addKey(pair.first);
      addVector(pair.second, wrap);
      index++;
      if (index != (int)values.size()) file << ",";
    }
    endObject();
  }

  void saveObject(std::unordered_map<std::string, MethodRuntimeSummary> & values) {
    startObject();
    int index = 0;
    for (auto & item : values) {
      addKey(item.first);
      startObject();

      addKey("runtime");
      addValue(item.second.runtime);
      file << ",";
      addKey("effect");
      double effect = ((item.second.runtime / totalRuntime) * 100);
      addValue(effect);
      file << ",";
      addKey("rir2pir");
      addValue(item.second.rir2pir);
      file << ",";
      addKey("hast");
      addValue(item.second.hast);
      file << ",";
      addKey("opt");
      addValue(item.second.opt);

      endObject();
      index++;
      if (index != (int)values.size()) file << ",";
    }
    endObject();
  }

  void saveObject(std::unordered_map<std::string, std::vector<RshMethod>> & values) {
    startObject();
    int index = 0;
    for (auto & item : values) {
      addKey(item.first);
      addVector(item.second);
      index++;
      if (index != (int)values.size()) file << ",";
    }
    endObject();
  }

  void saveObject(std::unordered_map<std::string, Meta> & values) {
    startObject();
    int index = 0;
    for (auto & item : values) {
      addKey(item.first);
      file << "{";

      addKey("name");
      file << "\"";
      addValue(item.second.name);
      file << "\"";
      file << ",";

      addKey("runs");
      addValue(item.second.runs);

      file << "}";
      index++;
      if (index != (int)values.size()) file << ",";
    }
    endObject();
  }

  void saveObject(std::unordered_map<std::string, ContextRuntimeSummary> & values) {
    startObject();
    int index = 0;
    for (auto & item : values) {
      addKey(item.first);


      file << "{";

      addKey("runs");
      addValue(item.second.runs);
      file << ",";

      addKey("cmp");
      addValue(item.second.cmp);
      file << ",";

      addKey("opt");
      addValue(item.second.opt);
      file << ",";

      addKey("run");
      addValue(item.second.run);

      file << "}";


      index++;
      if (index != (int)values.size()) file << ",";
    }
    endObject();
  }

  void saveObject(std::unordered_map<std::string, std::unordered_map<std::string, std::vector<double>>> & values) {
    startObject();
    int index = 0;
    for (auto & item : values) {
      addKey(item.first);
      saveObject(item.second);
      index++;
      if (index != (int)values.size()) file << ",";
    }
    endObject();
  }

  void saveObject(std::unordered_map<std::string, std::unordered_map<std::string, ContextRuntimeSummary>> & values) {
    startObject();
    int index = 0;
    for (auto & item : values) {
      addKey(item.first);
      saveObject(item.second);
      index++;
      if (index != (int)values.size()) file << ",";
    }
    endObject();
  }

  double totalRuntime;


  public:
  JsonWriter(std::string & path) {
    file.open(path);
    newLine = false;
  }

  JsonWriter(std::string & path, bool printNewLine) : newLine(printNewLine) {
    file.open(path);
  }

  ~JsonWriter() {
    file.close();
  }

  void addTotalRuntime(const double & t) {
    totalRuntime = t;
    R_totalRuntime = t;
  }

  void start() {
    file << "{";
    if (newLine) file << "\n";
  }

  void end() {
    if (newLine) file << "\n";
    file << "}";
  }

  void addKeyValue(const std::string & key, const std::string & value, bool last) {
    addKey(key);
    addValue(value);
    if (!last) file << ",";
  }

  void addKeyValue(const std::string & key, const double & value, bool last) {
    addKey(key);
    addValue(value);
    if (!last) file << ",";
  }

  void saveBoxContextResolution(const std::string & key, std::unordered_map<std::string, int> & values, bool last) {
    addKey(key);
    saveObject(values, false);
    if (!last) file << ",";
    if (newLine) file << "\n";
  }

  void saveBoxContextCallRuns(const std::string & key, std::unordered_map<std::string, std::vector<int>> & values, bool last) {
    addKey(key);
    saveObject(values, false);
    if (!last) file << ",";
    if (newLine) file << "\n";
  }

  void saveMethodCallMap(const std::string & key, std::unordered_map<std::string, std::set<std::string>> & values, bool last) {
    addKey(key);
    saveObject(values, false);
    if (!last) file << ",";
    if (newLine) file << "\n";
  }

  void saveMethodContextMap(const std::string & key, std::unordered_map<std::string, std::set<std::string>> & values, bool last) {
    addKey(key);
    saveObject(values, true);
    if (!last) file << ",";
    if (newLine) file << "\n";
  }

  void saveMethodRuntimeMap(const std::string & key, std::unordered_map<std::string, MethodRuntimeSummary> & values, bool last) {
    addKey(key);
    saveObject(values);
    if (!last) file << ",";
    if (newLine) file << "\n";
  }

  void saveMethodsThatCompiledMap(const std::string & key, std::unordered_map<std::string, std::vector<RshMethod>> & values, bool last) {
    addKey(key);
    saveObject(values);
    if (!last) file << ",";
    if (newLine) file << "\n";
  }

  void saveMethodMeta(const std::string & key, std::unordered_map<std::string, Meta> & values, bool last) {
    addKey(key);
    saveObject(values);
    if (!last) file << ",";
    if (newLine) file << "\n";
  }

  void saveMethodCSMap(const std::string & key, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<double>>> & values, bool last) {
    addKey(key);
    saveObject(values);
    if (!last) file << ",";
    if (newLine) file << "\n";
  }

  void saveMethodCTMap(const std::string & key, std::unordered_map<std::string, std::unordered_map<std::string, ContextRuntimeSummary>> values, bool last) {
    addKey(key);
    saveObject(values);
    if (!last) file << ",";
    if (newLine) file << "\n";
  }

  void saveSort(const std::string & key, std::vector<std::string> values, bool last) {
    addKey(key);
    addVector(values, true);
    if (!last) file << ",";
    if (newLine) file << "\n";
  }

};
