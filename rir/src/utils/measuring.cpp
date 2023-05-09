#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

#include "utils/measuring.h"
#include "RshViz.h"

std::string del = ",";
bool isStepped = false;
uintptr_t pcc = 0;
RshJsonParser parser(del, 0);
std::unordered_map<size_t, std::set<std::string>> mtoc;
std::unordered_map<size_t, std::unordered_map<std::string, size_t>> mtocr;

std::string& removeQuotes(std::string& str) {
    str.erase(0, 1);
    str.erase(str.size() - 1);
    return str;
}

std::ostream& operator<<(std::ostream& os, const RshMethod& m) {
    std::string del = ",";
    os << "id: " << m.id << del << " name: " << m.name << del
       << " context: " << m.context << del << " compiled: " << m.compiled << del
       << " rir2pir: " << m.rir2pir << del << " opt: " << m.opt << del
       << " bb: " << m.bb << del << " p: " << m.p << del
       << " failed: " << m.failed << del << " runtime: " << m.runtime << del
       << " effect: " << m.effect << del << " hast: " << m.hast << del
       << " contextI: " << m.contextI;
    return os;
}

void (*singleLineObjectSerializer)(RshMethod&, std::ofstream&, std::string&) =
    [](RshMethod& m, std::ofstream& os, std::string& del) {
        os << m.id << del << m.name << del << m.context << del << m.compiled
           << del << m.rir2pir << del << m.opt << del << m.bb << del << m.p
           << del << m.failed << del << m.runtime << del << m.effect << del
           << m.hast << del << m.contextI;
        os << std::endl;
    };

double R_totalRuntime = 0.0;

void (*lineToObjectParser)(RshMethod&, std::string&, std::string&) =
    [](RshMethod& method, std::string& tp, std::string& del) {
        int start = 0;
        int end = tp.find(del);
        int index = 0;
        while (end != -1) {
            switch (index) {
            case 0:
                method.id = tp.substr(start, end - start);
                break;
            case 1:
                method.name = tp.substr(start, end - start);
                break;
            case 2:
                method.context = tp.substr(start, end - start);
                break;
            case 4:
                method.compiled = std::stoi(tp.substr(start, end - start));
                break;
            case 5:
                method.rir2pir = std::stod(tp.substr(start, end - start));
                break;
            case 6:
                method.opt = std::stod(tp.substr(start, end - start));
                break;
            case 7:
                method.bb = std::stoi(tp.substr(start, end - start));
                break;
            case 8:
                method.p = std::stoi(tp.substr(start, end - start));
                break;
            case 9:
                method.failed = std::stod(tp.substr(start, end - start));
                break;
            case 10:
                method.runtime = std::stod(tp.substr(start, end - start));
                break;
            case 11:
                method.effect = std::stod(tp.substr(start, end - start));
                break;
            case 12:
                method.hast = tp.substr(start, end - start);
                break;
            case 13:
                method.contextI = std::stoul(tp.substr(start, end - start));
                break;
            }
            start = end + del.size();
            end = tp.find(del, start);
            index++;
        }
    };

namespace rir {

namespace {

struct MeasuringImpl {
    struct Timer {
        double timer = 0;
        bool timerActive = false;
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        size_t alreadyRunning = 0;
        size_t notStarted = 0;
    };
    std::unordered_map<std::string, Timer> timers;
    std::unordered_map<std::string, size_t> events;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
#if LOGG > 0
    std::ofstream logg_stream;
    std::chrono::time_point<std::chrono::steady_clock> c_start, c_end;
#endif
    size_t threshold = 0;
    const unsigned width = 40;
    bool shouldOutput = false;

    MeasuringImpl() : start(std::chrono::high_resolution_clock::now()) {
#if LOGG > 0
        c_start = std::chrono::steady_clock::now();

        if (getenv("LOGG") != NULL) {
            std::string binId = getenv("LOGG");
            logg_stream.open(binId + ".logg");
        } else {
            time_t timenow = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
            std::stringstream runId_ss;
            runId_ss << std::put_time(localtime(&timenow), "%FT%T%z")
                     << ".logg";
            logg_stream.open(runId_ss.str());
        }

#endif
    }

    ~MeasuringImpl() {

        RshViz::_eventLock.lock();
        RshViz::current_socket->emit(RshViz::APP_END_PROG);
        RshViz::_eventLock.unlock();
        std::cout << "Program Ended..............."<< std::endl;
        end = std::chrono::high_resolution_clock::now();
        auto logfile = getenv("PIR_MEASURING_LOGFILE");
        if (logfile) {
            std::ofstream fs(logfile);
            if (!fs) {
                std::cerr << "ERROR: Can't open logfile '" << logfile
                          << "'. Writing to std::cerr.\n";
                dump(std::cerr);
            } else {
                dump(fs);
            }
        } else {
            dump(std::cerr);
        }
#if LOGG > 0
        c_end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> total = end - start;
        std::string str;
        str = "##," + std::to_string(total.count());
        parser.processLine(str);
        // std::string outputPath =
        // "/home/aayush/rir_viz/rir/build/logg_json.json";
        time_t timenow = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        std::stringstream runId_ss;
        runId_ss << std::put_time(localtime(&timenow), "%FT%T%z") << ".json";
        parser.saveVizJson(runId_ss.str());
        logg_stream << "##," << total.count();
        logg_stream.close();
#endif
    }

    void dump(std::ostream& out) {
        if (!shouldOutput)
            return;

        std::chrono::duration<double> duration = end - start;
        out << "\n---== Measuring breakdown ===---\n\n";
        out << "  Total lifetime: " << format(duration.count()) << "\n\n";

        {
            std::map<double, std::tuple<std::string, size_t, size_t, double>>
                orderedTimers;
            double totalTimers = 0;
            for (auto& t : timers) {
                auto& key = t.second.timer;
                while (orderedTimers.count(key))
                    key += 1e-20;
                double notStopped = 0;
                if (t.second.timerActive) {
                    duration = end - t.second.start;
                    notStopped = duration.count();
                }
                orderedTimers.emplace(
                    key, std::make_tuple(t.first, t.second.alreadyRunning,
                                         t.second.notStarted, notStopped));
                totalTimers += key;
            }
            if (!orderedTimers.empty()) {
                out << "  Timers (" << format(totalTimers) << " in total, or "
                    << std::setprecision(2)
                    << (totalTimers / duration.count() * 100) << "%):\n";
                for (auto& t : orderedTimers) {
                    auto& name = std::get<0>(t.second);
                    out << "    " << std::setw(width) << name << "\t"
                        << format(t.first);
                    if (auto& alreadyRunning = std::get<1>(t.second)) {
                        out << "  (started " << alreadyRunning
                            << "x while running)!";
                    }
                    if (auto& notStarted = std::get<2>(t.second)) {
                        out << "  (counted " << notStarted
                            << "x while not running)!";
                    }
                    if (auto& notStopped = std::get<3>(t.second)) {
                        out << "  (not stopped, measured extra "
                            << format(notStopped) << ")!";
                    }
                    out << "\n";
                }
                out << "\n";
            }
        }

        {
            std::map<size_t, std::set<std::string>> orderedEvents;
            for (auto& e : events)
                if (e.second >= threshold)
                    orderedEvents[e.second].insert(e.first);
            if (!orderedEvents.empty()) {
                out << "  Events";
                if (threshold)
                    out << " (threshold " << threshold << ")";
                out << ":\n";
                for (auto& e : orderedEvents) {
                    for (auto& n : e.second) {
                        out << "    " << std::setw(width) << n << "\t"
                            << e.first << " (~ " << readable(e.first) << ")\n";
                    }
                }
            }
        }

        out << std::flush;
    }

    static std::string format(double secs) {
        std::stringstream ss;
        if (secs < 60)
            ss << secs << " secs";
        else if (secs < 60 * 60)
            ss << secs / 60 << " min";
        else
            ss << secs / 60 / 60 << " hrs";
        return ss.str();
    }

    static std::string readable(size_t n) {
        std::stringstream ss;
        if (n > 1000UL * 1000UL * 1000UL * 1000UL)
            ss << (n / 1000 / 1000 / 1000 / 1000) << "T";
        else if (n > 1000UL * 1000UL * 1000UL)
            ss << (n / 1000 / 1000 / 1000) << "G";
        else if (n > 1000UL * 1000UL)
            ss << (n / 1000 / 1000) << "M";
        else if (n > 1000UL)
            ss << (n / 1000) << "K";
        else
            ss << n;
        return ss.str();
    }
};

} // namespace

std::unique_ptr<MeasuringImpl> m = std::make_unique<MeasuringImpl>();

void Measuring::startTimer(const std::string& name) {
    m->shouldOutput = true;
    auto& t = m->timers[name];
    if (t.timerActive) {
        t.alreadyRunning++;
    } else {
        t.timerActive = true;
        t.start = std::chrono::high_resolution_clock::now();
    }
}

void Measuring::countTimer(const std::string& name) {
    auto end = std::chrono::high_resolution_clock::now();
    m->shouldOutput = true;
    auto& t = m->timers[name];
    if (!t.timerActive) {
        t.notStarted++;
    } else {
        t.timerActive = false;
        std::chrono::duration<double> duration = end - t.start;
        Measuring::addTime(name, duration.count());
    }
}

void Measuring::addTime(const std::string& name, double time) {
    m->shouldOutput = true;
    m->timers[name].timer += time;
}

void Measuring::setEventThreshold(size_t n) {
    m->shouldOutput = true;
    m->threshold = n;
}

void Measuring::countEvent(const std::string& name, size_t n) {
    m->shouldOutput = true;
    m->events[name] += n;
}

#if LOGG > 0
std::ofstream& Measuring::getLogStream() { return m->logg_stream; }
#endif

void Measuring::reset(bool outputOld) {
    if (m)
        m->shouldOutput = outputOld;
    m.reset(new MeasuringImpl());
}

} // namespace rir
