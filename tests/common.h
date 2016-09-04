#pragma once
#include <stdio.h>
#include "../source/pngenc/utils.h"

#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        printf("Asserion failed in %s:%d\n", __FILE__, __LINE__); \
        return -1; \
    }

#define ASSERT_FALSE(expr) \
    if (expr) { \
        printf("Assertion failed in %s:%d\n", __FILE__, __LINE__); \
        return -1; \
    }
