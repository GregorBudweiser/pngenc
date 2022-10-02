#include "adler32.h"
#include <stdlib.h>

uint32_t adler_update64(uint32_t adler, const uint8_t * data, uint32_t length) {
    uint32_t s1 = adler & 0xFFFF;
    uint32_t s2 = adler >> 16;

    // align to 8 byte
    while(length && (((size_t)data & 0x7) != 0)) {
        s1 = (s1 + data[0]) % 65521;
        s2 = (s2 + s1) % 65521;
        data++;
        length--;
    }

    const uint32_t length8 = length >> 3;
    const uint64_t *data64 = (const uint64_t*)data;

    uint32_t n = 0;
    uint32_t k = 0;
    while(n < length8) {
        n += 690; // ~5550 / 8 -> as many iterations without modulo as possible
        if(n > length8)
            n = length8;

        // compute 8 bytes in one iteration
        while(k < n) {
            register uint64_t currentData = data64[k++];
            register uint8_t u0 = currentData & 0xFF;
            s1 = (s1 + u0);
            s2 = (s2 + s1);

            register uint8_t u1 = (currentData >> 8) & 0xFF;
            s1 = (s1 + u1);
            s2 = (s2 + s1);

            register uint8_t u2 = (currentData >> 16) & 0xFF;
            s1 = (s1 + u2);
            s2 = (s2 + s1);

            register uint8_t u3 = (currentData >> 24) & 0xFF;
            s1 = (s1 + u3);
            s2 = (s2 + s1);

            register uint8_t u4 = (currentData >> 32) & 0xFF;
            s1 = (s1 + u4);
            s2 = (s2 + s1);

            register uint8_t u5 = (currentData >> 40) & 0xFF;
            s1 = (s1 + u5);
            s2 = (s2 + s1);

            register uint8_t u6 = (currentData >> 48) & 0xFF;
            s1 = (s1 + u6);
            s2 = (s2 + s1);

            register uint8_t u7 = (currentData >> 56);
            s1 = (s1 + u7);
            s2 = (s2 + s1);
        }

        s1 = s1 % 65521;
        s2 = s2 % 65521;
    }

    // padding..
    for(n = length8 << 3; n < length; n++) {
        s1 = (s1 + data[n]) % 65521;
        s2 = (s2 + s1) % 65521;
    }

    return s1 | (s2 << 16);
}

uint32_t adler_update32(uint32_t adler, const uint8_t * data, uint32_t length) {
    uint32_t s1 = adler & 0xFFFF;
    uint32_t s2 = adler >> 16;

    // align to 8 byte
    while(length && (((size_t)data & 0x7) != 0)) {
        s1 = (s1 + data[0]) % 65521;
        s2 = (s2 + s1) % 65521;
        data++;
        length--;
    }

    const register uint32_t length8 = length >> 2;
    const uint32_t *data64 = (const uint32_t*)data;

    uint32_t n = 0;
    uint32_t k = 0;
    while(n < length8) {
        n += 2*690; // ~5550 / 4 -> as many iterations without modulo as possible
        if(n > length8)
            n = length8;

        // compute 8 bytes in one iteration
        while(k < n) {
            uint32_t currentData = data64[k++];
            uint8_t u0 = currentData & 0xFF;
            s1 = (s1 + u0);
            s2 = (s2 + s1);

            uint8_t u1 = (currentData >> 8) & 0xFF;
            s1 = (s1 + u1);
            s2 = (s2 + s1);

            uint8_t u2 = (currentData >> 16) & 0xFF;
            s1 = (s1 + u2);
            s2 = (s2 + s1);

            uint8_t u3 = currentData >> 24;
            s1 = (s1 + u3);
            s2 = (s2 + s1);
        }

        s1 = s1 % 65521;
        s2 = s2 % 65521;
    }

    // padding..
    for(n = length8 << 2; n < length; n++) {
        s1 = (s1 + data[n]) % 65521;
        s2 = (s2 + s1) % 65521;
    }

    return s1 | (s2 << 16);
}

uint32_t adler_update(uint32_t adler, const uint8_t * data, uint32_t length) {
    if(sizeof(size_t) == 8) {
        return adler_update64(adler, data, length);
    } else {
        return adler_update32(adler, data, length);
    }
}

/*
 * Taken and adapted from zlib
 */
const int BASE = 65521U;
uint32_t adler32_combine(uint32_t adler1, uint32_t adler2, size_t len2)
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
