#include "adler32.h"
#include "utils.h"
#include <stdint.h>

#if PNGENC_X86
#include <immintrin.h>

/*
 * Adapted from https://github.com/wooosh/fpng/
 */
uint32_t adler_update_hw(uint32_t adler, const uint8_t * data, size_t len) {
    const size_t N = 64;
    const __m256i ZERO = _mm256_setzero_si256();
    const __m256i ONE = _mm256_set1_epi16(1);
    const __m256i COEFF_0 = _mm256_set_epi8(
        33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, 52, 53, 54, 55, 56,
        57, 58, 59, 60, 61, 62, 63, 64
    );
    const __m256i COEFF_1 = _mm256_set_epi8(
        1,  2,  3,  4,  5,  6,  7,  8,
        9,  10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32
    );

    uint32_t sum1 = adler & 0xFFFF;
    uint32_t sum2 = adler >> 16;

    const size_t MAX_CHUNK_SIZE = (5552/N)*N;
    while (len >= N) {
        size_t chunk_len = len - (len % N);
        if (chunk_len > MAX_CHUNK_SIZE) {
            chunk_len = MAX_CHUNK_SIZE;
        }
        len -= chunk_len;

        __m256i s1 = _mm256_setzero_si256();
        __m256i s2 = _mm256_setzero_si256();
        __m256i s1_fold = _mm256_setzero_si256();

        const uint8_t *chunk_end = data + chunk_len;
        for (;data < chunk_end; data += N) {
            s1_fold = _mm256_add_epi64(s1_fold, s1);

            __m256i cur0 = _mm256_loadu_si256(data);
            __m256i cur1 = _mm256_loadu_si256(data + 32);

            // multiply each byte by the coefficient, and sum adjacent bytes into 16 bit integers
            __m256i mad0 = _mm256_maddubs_epi16(cur0, COEFF_0);
            __m256i mad1 = _mm256_maddubs_epi16(cur1, COEFF_1);
            s2 = _mm256_add_epi32(s2, _mm256_madd_epi16(mad0, ONE));
            s2 = _mm256_add_epi32(s2, _mm256_madd_epi16(mad1, ONE));

            // sum every consecutive 8 bytes together into 4 64-bit integers, then add to s1
            s1 = _mm256_add_epi32(s1, _mm256_sad_epu8(cur0, ZERO));
            s1 = _mm256_add_epi32(s1, _mm256_sad_epu8(cur1, ZERO));
        }

        // add 64*s1_fold to s2
        s2 = _mm256_add_epi32(s2, _mm256_slli_epi32(s1_fold, 6));
        sum2 += sum1 * chunk_len;

        // horizontal sum s1
        {
            __m256i hsum = _mm256_hadd_epi32(s1, ZERO);
            hsum = _mm256_hadd_epi32(hsum, ZERO);

            __m256i hsum2 = _mm256_permute2f128_si256(hsum, hsum, 0b10000001);
            hsum2 = _mm256_add_epi32(hsum, hsum2);
            sum1 += _mm256_cvtsi256_si32(hsum2);
        }

        // horizontal sum s2
        {
            __m256i hsum = _mm256_hadd_epi32(s2, ZERO);
            hsum = _mm256_hadd_epi32(hsum, ZERO);

            __m256i hsum2 = _mm256_permute2f128_si256(hsum, hsum, 0b10000001);
            hsum2 = _mm256_add_epi32(hsum, hsum2);
            sum2 += _mm256_cvtsi256_si32(hsum2);
        }

        sum1 %= 65521;
        sum2 %= 65521;
    }
    return adler_update64((sum2 << 16) | sum1, data, len);
}
#endif

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

uint32_t adler_update(uint32_t adler, const uint8_t * data, uint32_t len) {
#if PNGENC_X86
    if(has_avx2()) {
        return adler_update_hw(adler, data, len);
    }
#endif

    if(sizeof(size_t) == 8) {
        return adler_update64(adler, data, len);
    } else {
        return adler_update32(adler, data, len);
    }
}

/*
 * Taken and adapted from zlib
 */
uint32_t adler32_combine(uint32_t adler1, uint32_t adler2, size_t len2)
{
    const uint32_t BASE = 65521U;
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
