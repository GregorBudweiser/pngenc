#include "adler32.h"
#include <stdlib.h>

uint32_t adler_get_checksum(const pngenc_adler32 * adler) {
    return (adler->s2 << 16) | adler->s1;
}


void adler_set_checksum(pngenc_adler32 * adler, uint32_t checksum) {
    adler->s1 = checksum & 0xFF;
    adler->s2 = checksum >> 16;
}

void adler_init(pngenc_adler32 * adler) {
    adler->s1 = 1;
    adler->s2 = 0;
}

void adler_update(pngenc_adler32 * adler, const uint8_t * data,
                  uint32_t length) {
    if(sizeof(size_t) == 8) {
        adler_update64(adler, data, length);
    } else {
        adler_update32(adler, data, length);
    }
}

void adler_update64(pngenc_adler32 * adler, const uint8_t * data,
                    uint32_t length) {

    register uint32_t s1 = adler->s1;
    register uint32_t s2 = adler->s2;

    // align to 8 byte
    while(length && (((size_t)data & 0x7) != 0)) {
        s1 = (s1 + data[0]) % 65521;
        s2 = (s2 + s1) % 65521;
        data++;
        length--;
    }

    const register uint32_t length8 = length >> 3;
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

    adler->s1 = s1;
    adler->s2 = s2;
}

void adler_update32(pngenc_adler32 * adler, const uint8_t * data,
                    uint32_t length) {

    register uint32_t s1 = adler->s1;
    register uint32_t s2 = adler->s2;

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
            register uint32_t currentData = data64[k++];
            register uint8_t u0 = currentData & 0xFF;
            s1 = (s1 + u0);
            s2 = (s2 + s1);

            register uint8_t u1 = (currentData >> 8) & 0xFF;
            s1 = (s1 + u1);
            s2 = (s2 + s1);

            register uint8_t u2 = (currentData >> 16) & 0xFF;
            s1 = (s1 + u2);
            s2 = (s2 + s1);

            register uint8_t u3 = currentData >> 24;
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

    adler->s1 = s1;
    adler->s2 = s2;
}

void adler_copy_on_update(pngenc_adler32 * adler, const uint8_t * data,
                          uint32_t length, uint8_t * dst) {

    register uint32_t s1 = adler->s1;
    register uint32_t s2 = adler->s2;

    // align to 8 byte
    while(length && (((size_t)data & 0x7) != 0)) {
        s1 = (s1 + data[0]) % 65521;
        s2 = (s2 + s1) % 65521;
        *dst = *data;
        dst++;
        data++;
        length--;
    }

    const register uint32_t length8 = length >> 3;
    const uint64_t *data64 = (const uint64_t*)data;
    uint64_t *dst64 = (uint64_t*)dst;


    uint32_t n = 0;
    uint32_t k = 0;
    while(n < length8) {
        n += 690; // ~5550 / 8 -> as many iterations without modulo as possible
        if(n > length8)
            n = length8;

        // compute 8 bytes in one iteration
        while(k < n) {
            register uint64_t currentData = data64[k];
            dst64[k] = currentData;
            k++;

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
        dst[n] = data[n];
    }

    adler->s1 = s1;
    adler->s2 = s2;
}
