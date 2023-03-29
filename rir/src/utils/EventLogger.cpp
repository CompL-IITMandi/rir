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
#include <chrono>


using namespace std;
using namespace std::chrono;

namespace rir {

    static std::string logsFullPath = "";
    static std::string dispatchLogsFullPath = "";
    static std::string disableLogsFullPath = "";

    // timestamp
    // hast
    // event           - dispatch, deopt
    // isL2            - true/false
    // l1-context      - Level-1 context
    // l2-info         - Number of active L2 functions
    // l2-case         - fastcase/slowcase/miss/nil

    static void setDispatchLogsPath() {
        if (!dispatchLogsFullPath.size()) {
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

            ss << logsFolder << "/dispatch-" << runType << "-" << std::put_time(&tm, "%y%m%d-%H%M%S")
            << "-" << std::to_string(getpid()) << ".csv";
            dispatchLogsFullPath = ss.str();

            std::ofstream outfile;
            outfile.open(dispatchLogsFullPath, std::ios_base::app);
            outfile << "timestamp,hast,event,isL2,l1-context,l2-info,l2-case\n";
            outfile.close();
        }
    }

    static void setDisableLogsPath() {
        if (!disableLogsFullPath.size()) {
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

            ss << logsFolder << "/disable-" << runType << "-" << std::put_time(&tm, "%y%m%d-%H%M%S")
            << "-" << std::to_string(getpid()) << ".csv";
            disableLogsFullPath = ss.str();

            std::ofstream outfile;
            outfile.open(disableLogsFullPath, std::ios_base::app);
            outfile << "timestamp,hast,event,isL2,l1-context,l2-info,l2-case\n";
            outfile.close();
        }
    }
    void EventLogger::logDispatchNormal(SEXP hast, const rir::Context & context) {
        setDispatchLogsPath();
        std::ofstream outfile;
        outfile.open(dispatchLogsFullPath, std::ios_base::app);
        auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        outfile << timestamp << ",";
        outfile << (hast ? CHAR(PRINTNAME(hast)) : "") << ",";
        outfile << "dispatch" << ",";
        outfile << "false" << ",";
        outfile << context << ",";
        outfile << "0/0" << ",";
        outfile << "NIL" << ",";
        outfile << "\n";
        outfile.close();
    }

    void EventLogger::logDispatchL2(SEXP hast, const rir::Context & context, const std::string & l2Info) {
        setDispatchLogsPath();
        std::ofstream outfile;
        outfile.open(dispatchLogsFullPath, std::ios_base::app);
        auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        outfile << timestamp << ",";
        outfile << (hast ? CHAR(PRINTNAME(hast)) : "") << ",";
        outfile << "dispatch" << ",";
        outfile << "true" << ",";
        outfile << context << ",";
        outfile << l2Info << ",";
        outfile << "NIL" << ",";
        outfile << "\n";
        outfile.close();
    }

    void EventLogger::logL2Check(SEXP hast, const rir::Context & context, const std::string & l2Info, const char * l2Case) {
        setDispatchLogsPath();
        std::ofstream outfile;
        outfile.open(dispatchLogsFullPath, std::ios_base::app);
        auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        outfile << timestamp << ",";
        outfile << (hast ? CHAR(PRINTNAME(hast)) : "") << ",";
        outfile << "check" << ",";
        outfile << "true" << ",";
        outfile << context << ",";
        outfile << l2Info << ",";
        outfile << l2Case << ",";
        outfile << "\n";
        outfile.close();
    }

    void EventLogger::logDisable(SEXP hast, const rir::Context & context) {
        setDisableLogsPath();
        std::ofstream outfile;
        outfile.open(disableLogsFullPath, std::ios_base::app);
        auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        outfile << timestamp << ",";
        outfile << (hast ? CHAR(PRINTNAME(hast)) : "") << ",";
        outfile << "deopt" << ",";
        outfile << "false" << ",";
        outfile << context << ",";
        outfile << "0" << ",";
        outfile << "nil";
        outfile << "\n";
        outfile.close();

    }

    void EventLogger::logDisableL2(SEXP hast, const rir::Context & context, const std::string & l2Info) {
        setDisableLogsPath();
        std::ofstream outfile;
        outfile.open(disableLogsFullPath, std::ios_base::app);
        auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        outfile << timestamp << ",";
        outfile << (hast ? CHAR(PRINTNAME(hast)) : "") << ",";
        outfile << "deopt" << ",";
        outfile << "true" << ",";
        outfile << context << ",";
        outfile << l2Info << ",";
        outfile << "nil";
        outfile << "\n";
        outfile.close();
    }

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


    static void setStatsLogsPathIfNeeded() {
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
            << "-" << std::to_string(getpid()) << ".csv";
            logsFullPath = ss.str();

            // header
            std::ofstream outfile;
            outfile.open(logsFullPath, std::ios_base::app);
            outfile << "event,name,timeInMS,timeStamp,context,closure,size,pid";
            outfile << "\n";
            outfile.close();

        }

    }

    void EventLogger::logStats(
            std::string event,
            std::string name,
            double timeInMS,
            std::chrono::_V2::system_clock::time_point& timeStamp,
            const rir::Context& context,
            SEXP closure,
            size_t  size)
    {


        setStatsLogsPathIfNeeded();


        std::ofstream outfile;
        outfile.open(logsFullPath, std::ios_base::app);
        std::stringstream streamctx;
        streamctx << context;
        auto contextAsString = streamctx.str();
        findAndReplaceAll(contextAsString, ",", " ");

        outfile << event
                << "," << name
                << "," << timeInMS
                << "," << timeStamp.time_since_epoch().count()
                << "," << contextAsString
                << "," << closure
                << "," << size
                << "," << getpid();
        outfile << "\n";
        outfile.close();
    }

}
