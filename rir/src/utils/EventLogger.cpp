#include "EventLogger.h"



#include <iostream>
#include <unistd.h>
#include <sstream>


#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>


#include <fstream>
#include <iomanip>
#include <memory>


using namespace std;
using namespace std::chrono;

namespace rir {

    static std::string logsFullPath = "";

    void findAndReplaceAll(std::string & data, std::string toSearch, std::string replaceStr)
    {
        // Get the first occurrence
        size_t pos = data.find(toSearch);
        // Repeat till end is reached
        while( pos != std::string::npos)
        {
            // Replace this occurrence of Sub String
            data.replace(pos, toSearch.size(), replaceStr);
            // Get the next occurrence from the current position
            pos =data.find(toSearch, pos + replaceStr.size());
        }
    }


    void EventLogger::logStats(
            std::string event,
            std::string name,
            double timeInMS,
            std::chrono::_V2::system_clock::time_point& timeStamp,
            const rir::Context& context,
            SEXP closure)
    {

        if (!logsFullPath.size()) {
            std::string logsFolder ="/tmp/rsh";
            if (getenv("testResultsFolder"))
                logsFolder =  getenv("testResultsFolder");


            std::string runType ="noruntype";
            if (getenv("runType"))
                runType =  getenv("runType");

            struct stat st = {0};
            if (stat(logsFolder.c_str(), &st) == -1) {
                mkdir(logsFolder.c_str(),0700);
            }

            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);
            std::stringstream ss;

            ss << logsFolder << "/stats-" << runType << "-" << std::put_time(&tm, "%y%m%d-%H%M%S")
            << "-" << std::to_string(getpid()) << ".log";
            logsFullPath = ss.str();
        }

        std::ofstream outfile;
        outfile.open(logsFullPath, std::ios_base::app);

        std::stringstream streamctx;
        streamctx << context;
        auto contextAsString = streamctx.str();
        findAndReplaceAll(contextAsString, ",", " ");

        outfile << event << "," << name << "," << timeInMS << "," << timeStamp.time_since_epoch().count() << "," << contextAsString << "," << closure;
        outfile << "\n";
        outfile.close();
    }

}
