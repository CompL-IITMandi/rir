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

namespace rir {

    static std::string logsFullPath = "";


    void EventLogger::logStats(std::string event, std::string name, double timeInMS) {

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
        outfile << event << "," << name << "," << timeInMS;
        outfile << "\n";
        outfile.close();
    }

}
