#pragma once
#include "RshMethod.h"
#include <stdlib.h>
#include <time.h>
template <typename T>
#define MAX_RT_STACK_SIZE 1000000
class HugeStackManager {
  private:
  // The last temporary file used to store active stack information
  int last_temp_file_index;

  // Session identifier
  int session;

  // The runtime stack used to perform all the operations
  std::vector<T> rtStack;

  // callback to parse from a line of the stream
  void(*lineToObjectParser)(T &,std::string &,std::string &);

  // callback to write to stream
  void(*singleLineObjectSerializer)(T &,std::ofstream &,std::string &);

  // Delimiter for the temporary file
  std::string del;

  void flushToTemporary() {
    last_temp_file_index++; // increment the last index, this will make the file accessable when required
    std::ofstream outData;
    std::stringstream path;
    path << "/tmp/" << session << last_temp_file_index;
    outData.open(path.str());

    if( !outData ) { // file couldn't be opened
      std::cerr << "broken index" << last_temp_file_index << std::endl;
      exit(1);
    }

    for (auto & ele : rtStack) {
      singleLineObjectSerializer(ele,outData,del);
    }
    outData.close();
    std::cout << "data saved to temporary file";
    rtStack.clear();
  }

  void pullDataFromTemporary() {
    rtStack.clear();
    std::ifstream inData;
    std::stringstream path;
    path << "/tmp/" << session << last_temp_file_index;
    inData.open(path.str());

    if( !inData ) { // file couldn't be opened
      std::cerr << "broken index" << last_temp_file_index << std::endl;
      exit(1);
    }

    // temp file opened
    std::string tp;
    while(getline(inData, tp)){ //read data from file object and put it into string.
      T obj;
      lineToObjectParser(obj,tp,del);
      rtStack.push_back(obj);
    }

    inData.close(); //close the file object.

    std::cout << "data loaded from temporary file";

    // TODO - DELETE TEMPORARY FILE
    last_temp_file_index--; // decrement the file index
  }

  public:
  HugeStackManager(void(*l)(T &,std::string &,std::string &), void(*s)(T &,std::ofstream &,std::string &), std::string d = ",")
   : lineToObjectParser(l), singleLineObjectSerializer(s), del(d) {
    session = rand();
    rtStack.reserve(MAX_RT_STACK_SIZE);
    last_temp_file_index = -1;
  }

  void push(T & m) {
    if (rtStack.size() > MAX_RT_STACK_SIZE) {
      // flush stack data into a temporary file
      flushToTemporary();
    }
    rtStack.push_back(m);
  }

  T pop() {
    if (rtStack.empty() && last_temp_file_index > -1) {
      // pull data from temporary file
      pullDataFromTemporary();
    }
    T tmp = rtStack.back();
    rtStack.pop_back();
    return tmp;
  }

  bool empty() {
    return rtStack.empty() && last_temp_file_index == -1;
  }
};
