#include "adler32.h"

uint32_t adler_get_checksum(const pngenc_adler32 * adler) {
    return (adler->s2 << 16) | adler->s1;
}

void adler_init(pngenc_adler32 * adler) {
    adler->s1 = 1;
    adler->s2 = 0;
}

void adler_update(pngenc_adler32 * adler, const uint8_t * data,
                  const uint32_t length) {
    const register uint32_t length8 = length >> 3;
    const uint64_t *data64 = (const uint64_t*)data;

    register uint32_t s1 = adler->s1;
    register uint32_t s2 = adler->s2;

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
