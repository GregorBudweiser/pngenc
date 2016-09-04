#pragma once
#include <stdint.h>

uint64_t min_u64(uint64_t a, uint64_t b);

uint64_t min_i64(int64_t a, int64_t b);

uint32_t min_u32(uint32_t a, uint32_t b);

uint32_t max_u32(uint32_t a, uint32_t b);

uint32_t swap_endianness32(uint32_t value);

#define RETURN_ON_ERROR(expr) \
{ \
    int result = expr; \
    if(result) { \
        return result; \
    } \
}
