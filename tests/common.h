#pragma once
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include "../source/pngenc/utils.h"

#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        printf("Asserion failed in %s:%d\n", __FILE__, __LINE__); \
        printf("-- Expression: %s\n", #expr); \
        return -1; \
    }

#define ASSERT_FALSE(expr) \
    if (expr) { \
        printf("Assertion failed in %s:%d\n", __FILE__, __LINE__); \
        printf("-- Expression: %s\n", #expr); \
        return -1; \
    }


#if defined(WIN32) || defined(_WIN32)
#include <windows.h>

typedef uint64_t time_type;
static time_type get_timestamp() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

static double get_time_passed_ms(time_type prev) {
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
#endif

#define TIMING_START time_type _start = get_timestamp()
#define TIMING_END printf("Call to %s: %.02fms\n", __FUNCTION__, get_time_passed_ms(_start));
#define TIMING_END_MB(x) printf("Call to %s: %.02fGB/s\n", __FUNCTION__, x/get_time_passed_ms(_start));
