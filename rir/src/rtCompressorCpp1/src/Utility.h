#pragma once
#include <string>

template <typename Functor>
void handleLine(std::string & line, std::string del, Functor callback) {
  size_t pos = 0;
  std::string token;
  int index = 0;
  while ((pos = line.find(del)) != std::string::npos) {
    token = line.substr(0, pos);
    callback(index,token);
    line.erase(0, pos + del.length());
    index++;
  }
  callback(index,line);
}
std::string & removeQuotes(std::string & str);
