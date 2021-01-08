#pragma once
#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include "../source/pngenc/utils.h"

#define ASSERT_EQUAL(exprA, exprB) \
    if (!((exprA) == (exprB))) { \
        printf("Equality asserion failed in %s:%d\n", __FILE__, __LINE__); \
        printf("-- Expressions: %s == %s\n", #exprA, #exprB); \
        printf("-- Eval: %d == %d\n", (int)(exprA), (int)(exprB)); \
        return -1; \
    }

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
typedef uint64_t time_type;
#else
typedef struct timespec time_type;
#endif

time_type get_timestamp();
double get_time_passed_ms(time_type prev);

#define TIMING_START time_type _start = get_timestamp();
#define TIMING_END printf("Call to %s: %.02fms\n", __FUNCTION__, get_time_passed_ms(_start));
#define TIMING_END_MB(x) printf("Call to %s: %.02fGB/s\n", __FUNCTION__, x/get_time_passed_ms(_start));
