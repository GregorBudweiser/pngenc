#include "Timer.h"

#ifdef WIN32
#include <windows.h>
Timer::Timer() {
    reset();
}

Timer::~Timer() {
}

void Timer::reset() {
    // Get resolution
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    m_frequency = double(li.QuadPart);

    QueryPerformanceCounter(&li);
    m_last = li.QuadPart;
}

double Timer::getTimePassedNS() const {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-m_last)/m_frequency*1e9;
}

double Timer::getResolutionNS() const {
    return 1e9/m_frequency;
}
#else
Timer::Timer() {
    reset();
}

Timer::~Timer() {
}

void Timer::reset() {
    clock_gettime(CLOCK_MONOTONIC, &m_last);
}

double Timer::getTimePassedNS() const {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // http://tdistler.com/2010/06/27/high-performance-timing-on-linux-windows
    unsigned long deltaTimeSecs = (unsigned long)now.tv_sec - (unsigned long)m_last.tv_sec;

    // nanoseconds have to be between 0 and 999999999
    long deltaTimeNano = now.tv_nsec - m_last.tv_nsec;
    if(deltaTimeNano < 0) {
        deltaTimeNano = 1000000000 + deltaTimeNano;
        deltaTimeSecs--;
    }

    return double(deltaTimeNano) + double(deltaTimeSecs)*1E9;
}

double Timer::getResolutionNS() const {
    struct timespec res;
    clock_getres(CLOCK_MONOTONIC, &res);
    return double(res.tv_nsec) + double(res.tv_sec)*1E9;
}
#endif
