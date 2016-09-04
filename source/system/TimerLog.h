#ifndef TIMERLOG_H
#define TIMERLOG_H

#include <string>
#include "Timer.h"
#include "Log.h"

class TimerLog
{
public:
    TimerLog(const std::string & text)
        : m_text(text)
    {
        Log::s_log.log(Log::LT_DEFAULT, "Called: %s", m_text.c_str());
        Log::s_log.push();

        m_timer.reset();
    }

    ~TimerLog()
    {
        Log::s_log.pop();
        Log::s_log.log(Log::LT_DEFAULT, "+--[%7.03fms]", m_timer.getTimePassedNS()*1e-6);
        Log::s_log.log(Log::LT_DEFAULT, "");
    }

private:
    Timer m_timer;
    std::string m_text;
};

#define ENABLE_PERF_LOG

#ifdef ENABLE_PERF_LOG
    #ifdef WIN32
    #define LOG_TIME() TimerLog myTimerLog(__FUNCTION__);
    #else
    #define LOG_TIME() TimerLog myTimerLog(__PRETTY_FUNCTION__);
    #endif
#else
#define LOG_TIME()
#endif

#endif // TIMERLOG_H
