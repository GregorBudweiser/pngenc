#include "crc32.h"

// See: http://stackoverflow.com/questions/17645167/implementing-sse-4-2s-crc32c-in-software

/* crc32c.c -- compute CRC-32C using the Intel crc32 instruction
 * Copyright (C) 2013 Mark Adler
 * Version 1.1  1 Aug 2013  Mark Adler
 */

/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler
  madler@alumni.caltech.edu
 */

/* Use hardware CRC instruction on Intel SSE 4.2 processors.  This computes a
   CRC-32C, *not* the CRC-32 used by Ethernet and zip, gzip, etc.  A software
   version is provided as a fall-back, as well as for speed comparisons. */

/* Version history:
   1.0  10 Feb 2013  First version
   1.1   1 Aug 2013  Correct comments on why three crc instructions in parallel
 */

/* Adaption for PNG-compatible crc32 by Gregor Budweiser
   May, 2016
   - Removed hw path
   - PNG crc32 polynom: 0xEDB88320
   Oct, 2022
   - Implemented hw path based on intel paper and fpng
 */

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "utils.h"

#if PNGENC_X86
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <smmintrin.h>
#include <wmmintrin.h>
#endif

#ifdef _MSC_VER
    #define ALIGN(x) __declspec(align(x))
#else
    #define ALIGN(x)  __attribute__((aligned(16)))
#endif

/* CRC-32 (zlib) polynomial in reversed bit order. */
#define POLY 0xEDB88320

static uint32_t crc32c_table[8][256];

/* Construct table for software CRC-32C calculation. */
void crc32_init_sw() {
    uint32_t n, crc, k;

    for (n = 0; n < 256; n++) {
        crc = n;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc32c_table[0][n] = crc;
    }
    for (n = 0; n < 256; n++) {
        crc = crc32c_table[0][n];
        for (k = 1; k < 8; k++) {
            crc = crc32c_table[0][crc & 0xff] ^ (crc >> 8);
            crc32c_table[k][n] = crc;
        }
    }
}

/* Table-driven software version as a fall-back. This is about 15 times slower
   than using the hardware instructions.  This assumes little-endian integers,
   as is the case on Intel processors that the assembler code here is for. */
uint32_t crc32_sw(uint32_t crci, const void *buf, size_t len) {
    const unsigned char *next = (const unsigned char*)buf;
    uint64_t crc;
    crc = crci;
    while (len && ((uintptr_t)next & 7) != 0) {
        crc = crc32c_table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
        len--;
    }
    while (len >= 8) {
        crc ^= *(const uint64_t *)next;
        crc = crc32c_table[7][crc & 0xff] ^
              crc32c_table[6][(crc >> 8) & 0xff] ^
              crc32c_table[5][(crc >> 16) & 0xff] ^
              crc32c_table[4][(crc >> 24) & 0xff] ^
              crc32c_table[3][(crc >> 32) & 0xff] ^
              crc32c_table[2][(crc >> 40) & 0xff] ^
              crc32c_table[1][(crc >> 48) & 0xff] ^
              crc32c_table[0][crc >> 56];
        next += 8;
        len -= 8;
    }
    while (len) {
        crc = crc32c_table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
        len--;
    }
    return (uint32_t)crc;
}


uint32_t crc32_simple(uint32_t crc, const uint8_t *buf, size_t len) {
    static int i = 0;
    if (i == 0) {
        crc32_init_sw();
        i++;
    }

    for(size_t i = 0; i < len; i++) {
        crc = crc32c_table[0][(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    }
    return crc;
}

#if PNGENC_X86
// see: https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/fast-crc-computation-generic-polynomials-pclmulqdq-paper.pdf
uint32_t crc32_pclmul(uint32_t crci, const uint8_t* buf, size_t len) {
    assert(((size_t)buf) & ~63);
    assert((len & 63) == 0);

    static const uint64_t ALIGN(16) PX_U[2]  = { 0x1db710641, 0x1f7011641 };
    static const uint64_t ALIGN(16) K1_K2[2] = { 0x154442bd4, 0x1c6e41596 };
    static const uint64_t ALIGN(16) K3_K4[2] = { 0x1751997d0, 0x0ccaa009e };
    static const uint64_t ALIGN(16) K5[2]    = { 0x163cd6124, 0 };

    __m128i crc0 = _mm_xor_si128(_mm_cvtsi32_si128(crci), _mm_load_si128((const __m128i*)buf));
    __m128i crc1 = _mm_load_si128((const __m128i*)(buf+16));
    __m128i crc2 = _mm_load_si128((const __m128i*)(buf+32));
    __m128i crc3 = _mm_load_si128((const __m128i*)(buf+48));

    // T = 512 step
    const __m128i k1_k2 = _mm_load_si128((const __m128i*)K1_K2);
    for (size_t i = 64; i < len; i += 64) {
        __m128i cur0 = _mm_load_si128((const __m128i*)(buf + i));
        __m128i cur1 = _mm_load_si128((const __m128i*)(buf + i + 16));
        __m128i cur2 = _mm_load_si128((const __m128i*)(buf + i + 32));
        __m128i cur3 = _mm_load_si128((const __m128i*)(buf + i + 48));
        cur0 = _mm_xor_si128(cur0, _mm_clmulepi64_si128(crc0, k1_k2, 0x11));
        crc0 = _mm_xor_si128(cur0, _mm_clmulepi64_si128(crc0, k1_k2, 0x00));
        cur1 = _mm_xor_si128(cur1, _mm_clmulepi64_si128(crc1, k1_k2, 0x11));
        crc1 = _mm_xor_si128(cur1, _mm_clmulepi64_si128(crc1, k1_k2, 0x00));
        cur2 = _mm_xor_si128(cur2, _mm_clmulepi64_si128(crc2, k1_k2, 0x11));
        crc2 = _mm_xor_si128(cur2, _mm_clmulepi64_si128(crc2, k1_k2, 0x00));
        cur3 = _mm_xor_si128(cur3, _mm_clmulepi64_si128(crc3, k1_k2, 0x11));
        crc3 = _mm_xor_si128(cur3, _mm_clmulepi64_si128(crc3, k1_k2, 0x00));
    }

    // Reduce from 512 to 128 bits
    const __m128i k3_k4 = _mm_load_si128((const __m128i*)K3_K4);
    __m128i cur = crc0;
    cur = crc1;
    cur = _mm_xor_si128(cur, _mm_clmulepi64_si128(crc0, k3_k4, 0x11));
    crc0 = _mm_xor_si128(cur, _mm_clmulepi64_si128(crc0, k3_k4, 0x00));
    cur = crc2;
    cur = _mm_xor_si128(cur, _mm_clmulepi64_si128(crc0, k3_k4, 0x11));
    crc0 = _mm_xor_si128(cur, _mm_clmulepi64_si128(crc0, k3_k4, 0x00));
    cur = crc3;
    cur = _mm_xor_si128(cur, _mm_clmulepi64_si128(crc0, k3_k4, 0x11));
    crc0 = _mm_xor_si128(cur, _mm_clmulepi64_si128(crc0, k3_k4, 0x00));

    const __m128i mask = _mm_cvtsi32_si128(0xFFFFFFFF);
    const __m128i px_u = _mm_load_si128((const __m128i*)PX_U);
    const __m128i k5   = _mm_loadl_epi64((const __m128i*)K5);

    // Reduce from 128 to 64 bits
    __m128i R = _mm_xor_si128(_mm_srli_si128(crc0, 8), _mm_clmulepi64_si128(crc0, k3_k4, 0x10));          // (upper 8 bytes) ^ (lower 8 bytes * k4)
            R = _mm_xor_si128(_mm_srli_si128(R, 4), _mm_clmulepi64_si128(_mm_and_si128(R, mask), k5, 0)); // (upper 4 bytes) ^ (lower 4 bytes * k5)

    // Barrett Reduction
    const __m128i T1 = _mm_clmulepi64_si128(_mm_and_si128(R,  mask), px_u, 0x10); // T1' = R(x)' * u'
    const __m128i T2 = _mm_clmulepi64_si128(_mm_and_si128(T1, mask), px_u, 0x00); // T2' = T1' * P(x)'
    const __m128i C  = _mm_srli_si128(_mm_xor_si128(R, T2), 4);                   // C(x)' = (R(x)' ^ T2)/x^32
    return _mm_cvtsi128_si32(C);
}

uint32_t crc32_hw(uint32_t crc, const uint8_t* buf, size_t len) {
    const size_t align = 64;
    if (len < align) {
        return crc32_sw(crc, buf, len);
    }

    const uint8_t * aligned_buf = (const uint8_t*)((((size_t)buf)+(align-1)) & ~(align-1));
    const size_t aligned_start = aligned_buf - buf;
    const size_t aligned_len = (len-aligned_start) & ~(align-1);

    crc = crc32_sw(crc, buf, aligned_start);
    crc = crc32_pclmul(crc, aligned_buf, aligned_len);
    return crc32_sw(crc, buf + aligned_len + aligned_start, len - aligned_len - aligned_start);
}

uint32_t crc32(uint32_t crci, const void *buf, size_t len) {
    if (has_x86_clmul()) {
        return crc32_hw(crci, buf, len);
    } else {
        return crc32_sw(crci, buf, len);
    }
}
#else
uint32_t crc32(uint32_t crci, const void *buf, size_t len) {
    return crc32_sw(crci, buf, len);
}
#endif

