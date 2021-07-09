#include "rtCReg.h"
#include "rtC.h"
#include <chrono>
#include <ctime>
#include <stack>
#include <map>

#include "runtime/Context.h"
#include "interpreter/call_context.h"


namespace rir {

class TimeRegister {  // this class will be a singleton for a rirCall stack
private:
    std::stack<Timer> timerStack;
    std::stack<rtC_Entry> entryStack;
    double previousTime = 0;
    std::map<int, double> levelMap;
public:
    void startRecord(
        Timer t,
        rtC_Entry e
    ) {
        timerStack.push(t);
        entryStack.push(e);
        // std::cout << e.id << " -> PUSH: " << " s:" << timerStack.size() <<  std::endl;
    }

    void endRecord(bool deopt, std::string name) {
        rtC_Entry & e = entryStack.top();
        e.deopt = deopt;

        Timer & timr = timerStack.top();
        double clockTime = timr.tock();



        int currentLevel = timerStack.size();
        // get correct clocktime for the given method
        if ( levelMap.find(currentLevel + 1) != levelMap.end() ) {
            clockTime = clockTime - levelMap[currentLevel + 1];
        }
        double greaterLevelCumulative = 0;
        // invalidate all greater levels
        for (auto i = levelMap.begin(); i != levelMap.end(); i++) {
            if(i->first > currentLevel) {
                greaterLevelCumulative += levelMap[i->first];
                levelMap[i->first] = 0;
            }
        }
        // set the current level to be used by members lower in the stack
        if ( levelMap.find(currentLevel) != levelMap.end() ) {
            levelMap[currentLevel] = levelMap[currentLevel] + greaterLevelCumulative;
            levelMap[currentLevel] = levelMap[currentLevel] + clockTime;
        } else {
            levelMap[currentLevel] = clockTime + greaterLevelCumulative;
        }
        // set data for sending to the logger
        e.runtime = clockTime;

        // std::cout << e.id << " -> POP: " << ", runtime: " << e.runtime << " s:" << timerStack.size() <<  std::endl;

        rtC::addDispatchInfo(
            name,
            e.id,
            e.dispatched_context,
            e.baseline,
            e.deopt,
            e.runtime,
            e.tragedy,
            e.original_intention);
        entryStack.pop();
        timerStack.pop();

        if(timerStack.empty()) {
            // std::cout << "STACK EMPTY" << std::endl;
        }
    }

};

auto ref = std::make_unique<TimeRegister>();

void rtCReg::startRecord(
        Timer t,
        rtC_Entry e
    ) {
    ref->startRecord(t, e);
}

void rtCReg::endRecord(bool deopt, std::string name) {
    ref->endRecord(deopt, name);
}

};
