#pragma once
#include <stdint.h>
#include <stddef.h>

uint32_t adler_update(uint32_t adler, const uint8_t * data, uint32_t len);

uint32_t adler_update_hw(uint32_t adler, const uint8_t * data, size_t len);

/*
 * Taken and adapted from zlib
 */
uint32_t adler32_combine(uint32_t adler1, uint32_t adler2, size_t len2);
