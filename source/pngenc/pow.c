#include "pow.h"
#include <assert.h>

static const uint32_t pow95[] =
{
  0,
  1,
  1,
  3,
  7,
  13,
  26,
  51,
  100,
  194,
  374,
  724,
  1398,
  2702,
  5220,
  10085,
  19483,
  37640,
  72716,
  140479,
  271388,
  524288,
  1012857,
  1956712,
  3780118,
  7302707,
  14107901,
  27254668,
  52652548,
  101718016,
  196506256,
  379625056
};

const static uint32_t pow80[] = {
  0,
  1,
  1,
  3,
  5,
  9,
  16,
  27,
  48,
  84,
  147,
  256,
  445,
  776,
  1351,
  2352,
  4096,
  7131,
  12416,
  21618,
  37640,
  65536,
  114104,
  198668,
  345901,
  602248,
  1048576,
  1825676,
  3178688,
  5534417,
  9635980,
  16777216
};

/**
 * @returns The (one-based) index of the most significant bit set or zero if
 *          none is set. This function only works up to 2^31-1.
 */
uint32_t msb_set(uint32_t value) {
#if defined(__i386) || defined(__amd64) || defined(_M_AMD64)
    assert(value < 0x80000000); // max 31 bits
    value = (value << 1) | 1; // work around undefined result for 0 in clz
#if defined(_MSC_VER)
    return 31 - _BitScanReverse(value);
#else
    return 31 - __builtin_clz(value);
#endif
#else
    assert(value < 0x80000000); // max 31 bits

    for(int i = 30; i >= 0; i--) {
        if((1 << i) & value)
            return (uint32_t)i+1;
    }
    return 0;
#endif
}

/**
 * Lut based power approximation.
 *
 * Interpret value as "fixed-point"
 * value = 0....01,x...x
 *    a := 0....00,x...x (lerp alpha value)
 *    b := 0....01,0...0 - a (i.e. 1 - alpha)
 * result = lerp look-up table values.
 */
uint32_t fast_pow95(uint32_t value) {
    uint32_t msb = msb_set(value);
    uint64_t a = value & ~(1 << msb-1);
    uint64_t b = (1 << msb-1) - a;
    uint64_t lerp = pow95[msb+1]*a + pow95[msb]*b;
    return (uint32_t)(lerp >> msb-1);
}

/**
 * Same as fast_pow95 just a different lut.
 */
uint32_t fast_pow80(uint32_t value) {
    uint32_t msb = msb_set(value);
    uint64_t a = value & ~(1 << msb-1);
    uint64_t b = (1 << msb-1) - a;
    uint64_t lerp = pow80[msb+1]*a + pow80[msb]*b;
    return (uint32_t)(lerp >> msb-1);
}
