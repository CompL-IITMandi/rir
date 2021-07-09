
#ifndef RT_C_REG__H
#define RT_C_REG__H
#include "../interpreter/call_context.h"
#include <chrono>
#include <ctime>

namespace rir {

class Timer {
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start, m_End;
    std::chrono::duration<double> runtime;

    public:

        void tick() {
            m_Start = std::chrono::high_resolution_clock::now();
        }

        double tock() {
            m_End = std::chrono::high_resolution_clock::now();
            runtime = m_End - m_Start;
            return runtime.count();
        }
};

struct rtC_Entry {
    size_t id;
    Context dispatched_context;
    bool baseline;
    bool deopt;
    double runtime;
    bool tragedy;
    Context original_intention;
};

class rtCReg {
  public:
    static void startRecord(
        Timer t,
        rtC_Entry e
    );
    static void endRecord(bool deopt, std::string);
};

} // namespace rir

#endif
