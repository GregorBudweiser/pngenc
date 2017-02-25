#pragma once
#include <stdint.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

uint64_t min_u64(uint64_t a, uint64_t b);

int64_t min_i64(int64_t a, int64_t b);

uint32_t min_u32(uint32_t a, uint32_t b);

int32_t min_i32(int32_t a, int32_t b);

uint32_t max_u32(uint32_t a, uint32_t b);

int32_t max_i32(int32_t a, int32_t b);

float max_f32(float a, float b);

float max_f32(float a, float b);

uint32_t swap_endianness32(uint32_t value);

#define UNUSED(expr) do { (void)(expr); } while (0)

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
