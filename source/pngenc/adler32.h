#pragma once
#include <stdint.h>
#include <stddef.h>

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
uint32_t adler32_combine(uint32_t adler1, uint32_t adler2, size_t len2);
