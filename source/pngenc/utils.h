#pragma once
#include <stdint.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

uint64_t min_u64(uint64_t a, uint64_t b);

uint64_t min_i64(int64_t a, int64_t b);

uint32_t min_u32(uint32_t a, uint32_t b);

uint32_t max_u32(uint32_t a, uint32_t b);

uint32_t swap_endianness32(uint32_t value);

#ifndef NDEBUG
#define RETURN_ON_ERROR(expr) \
{ \
    int result = (int)expr; \
    if(result) { \
        printf("Returning on error: %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return result; \
    } \
}
#else
#define RETURN_ON_ERROR(expr) \
{ \
    int result = (int)expr; \
    if(result) { \
        return result; \
    } \
}
#endif
