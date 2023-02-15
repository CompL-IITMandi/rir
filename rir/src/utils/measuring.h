#ifndef MEASURING_H
#define MEASURING_H
#define LOGG 1
#include <string>
#include "../rtCompressorCpp/src/RshMethod.h"
#include "../rtCompressorCpp/src/RshJsonParser.h"


#if LOGG > 0
extern std::string del;
extern RshJsonParser parser;
extern std::unordered_map<size_t,std::set<std::string>> mtoc;
extern std::unordered_map<size_t, std::unordered_map<std::string,size_t>> mtocr;
#endif

namespace rir {

class Measuring {
  public:
    static void startTimer(const std::string& name);
    static void countTimer(const std::string& name);
    static void addTime(const std::string& name, double time);
    static void setEventThreshold(size_t n);
    static void countEvent(const std::string& name, size_t n = 1);
    #if LOGG > 0
    static std::ofstream & getLogStream();
    #endif
    static void reset(bool outputOld = false);
};

} // namespace rir

#endif
