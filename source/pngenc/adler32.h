#pragma once
#include <stdint.h>

typedef struct _pngenc_adler32 {
    uint32_t s1;
    uint32_t s2;
} pngenc_adler32;

void adler_init(pngenc_adler32 * adler);

void adler_update(pngenc_adler32 * adler, const uint8_t * data,
                  uint32_t length);
void adler_update32(pngenc_adler32 * adler, const uint8_t * data,
                    uint32_t length);
void adler_update64(pngenc_adler32 * adler, const uint8_t * data,
                    uint32_t length);

uint32_t adler_get_checksum(const pngenc_adler32 * adler);
void adler_set_checksum(pngenc_adler32 * adler, uint32_t checksum);
void adler_copy_on_update(pngenc_adler32 * adler, const uint8_t * data,
                          uint32_t length, uint8_t * dst);

/*
 * Taken and adapted from zlib
 */
const static int BASE = 65521U;
static uint32_t adler32_combine(uint32_t adler1, uint32_t adler2, size_t len2)
{
    uint32_t sum1;
    uint32_t sum2;
    uint32_t rem;

    /* the derivation of this formula is left as an exercise for the reader */
    len2 %= BASE;                /* assumes len2 >= 0 */
    rem = (uint32_t)len2;
    sum1 = adler1 & 0xffff;
    sum2 = rem * sum1;
    sum2 %= BASE;
    sum1 += (adler2 & 0xffff) + BASE - 1;
    sum2 += ((adler1 >> 16) & 0xffff) + ((adler2 >> 16) & 0xffff) + BASE - rem;
    if (sum1 >= BASE) sum1 -= BASE;
    if (sum1 >= BASE) sum1 -= BASE;
    if (sum2 >= ((unsigned long)BASE << 1)) sum2 -= ((unsigned long)BASE << 1);
    if (sum2 >= BASE) sum2 -= BASE;
    return sum1 | (sum2 << 16);
}
