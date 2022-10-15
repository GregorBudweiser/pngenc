#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#include <intrin.h>
#endif

uint64_t min_u64(uint64_t a, uint64_t b) {
    return a < b ? a : b;
}

int64_t min_i64(int64_t a, int64_t b) {
    return a < b ? a : b;
}

uint32_t min_u32(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

int32_t min_i32(int32_t a, int32_t b) {
    return a < b ? a : b;
}

uint32_t max_u32(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

int32_t max_i32(int32_t a, int32_t b) {
    return a > b ? a : b;
}

float max_f32(float a, float b) {
    return a > b ? a : b;
}

float min_f32(float a, float b) {
    return a < b ? a : b;
}

uint32_t swap_endianness32(uint32_t value) {
    return ((value & 0x000000FF) << 24) | ((value & 0x0000FF00) << 8)
         | ((value & 0x00FF0000) >> 8 ) | ((value & 0xFF000000) >> 24);
}

/**
 * Get index of highest bit set
 *
 * @return index or 0 if mask == 0
 */
uint32_t highest_bit_set(uint32_t mask) {
#if defined(_MSC_VER)
        uint32_t highest_bit;
        uint8_t was_non_zero = _BitScanReverse(&highest_bit, mask);
        return highest_bit * was_non_zero;
#else
        return (31 - __builtin_clz(mask)) * (mask != 0);
#endif
}

/**
 * Counts trailing zeros
 *
 * @return number of leading zero or 0 if mask == 0
 */
uint32_t count_trailing_zeros(uint32_t mask) {
#if defined(_MSC_VER)
        uint32_t trailing_zeros;
        uint8_t was_non_zero = _BitScanForward(&trailing_zeros, mask);
        return (31 - trailing_zeros) * was_non_zero;
#else
        return __builtin_ctz(mask) * (mask != 0);
#endif
}

/**
 * Counts leading zeros
 *
 * @return number of leading zero or 0 if mask == 0
 */
uint32_t count_leading_zeros(uint32_t mask) {
#if defined(_MSC_VER)
        uint32_t leading_zeros;
        uint8_t was_non_zero = _BitScanReverse(&leading_zeros, mask);
        return (31 - leading_zeros) * was_non_zero;
#else
        return __builtin_clz(mask) * (mask != 0);
#endif
}

static uint32_t x86_clmul = 0;
static uint32_t x86_avx2 = 0;

void init_cpu_info() {
#if PNGENC_X86
    uint32_t regs[4];
    uint32_t i = 1; // basic info
  #ifdef _MSC_VER
    __cpuid((int *)regs, (int)i);
  #else
    asm volatile
      ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
       : "a" (i), "c" (0));
  #endif
    x86_clmul = (regs[2] & (1 << 1)) > 0;

    i = 7;
  #ifdef _MSC_VER
    __cpuid((int *)regs, (int)i);
  #else
    asm volatile
      ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
       : "a" (i), "c" (0));
    // ECX is set to zero for CPUID function 4
  #endif
    x86_avx2  = (regs[1] & (1 << 5)) > 0;
#endif
}

uint32_t has_x86_clmul() {
    return x86_clmul;
}

uint32_t has_avx2() {
    return x86_avx2;
}
