#include "common.h"

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>

time_type get_timestamp() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

double get_time_passed_ms(time_type prev) {
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    double frequency = (double)li.QuadPart;
    QueryPerformanceCounter(&li);
    return (double)(li.QuadPart-prev)/(frequency*1E-3);
}
#else
time_type get_timestamp() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now;
}

double get_time_passed_ms(time_type prev) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // http://tdistler.com/2010/06/27/high-performance-timing-on-linux-windows
    unsigned long deltaTimeSecs = (unsigned long)now.tv_sec - (unsigned long)prev.tv_sec;

    // nanoseconds have to be between 0 and 999999999
    long deltaTimeNano = now.tv_nsec - prev.tv_nsec;
    if(deltaTimeNano < 0) {
        deltaTimeNano = 1000000000 + deltaTimeNano;
        deltaTimeSecs--;
    }

    return (double)deltaTimeNano*1E-6 + (double)deltaTimeSecs*1E3;
}

double Timer::getResolutionNS() const {
    struct timespec res;
    clock_getres(CLOCK_MONOTONIC, &res);
    return double(res.tv_nsec) + double(res.tv_sec)*1E9;
}
#endif
