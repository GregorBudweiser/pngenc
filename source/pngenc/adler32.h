#pragma once
#include <stdint.h>

typedef struct _pngenc_adler32 {
    uint32_t s1;
    uint32_t s2;
} pngenc_adler32;

void adler_init(pngenc_adler32 * adler);
void adler_update(pngenc_adler32 * adler, const uint8_t * data,
                  uint32_t length);
uint32_t adler_get_checksum(const pngenc_adler32 * adler);
