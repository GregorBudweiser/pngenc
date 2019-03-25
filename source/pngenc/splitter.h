#pragma once

#include "pngenc.h"

uint16_t swap_uint16(uint16_t val);

void push_bits(uint64_t bits, uint64_t nbits, uint8_t * data,
               uint64_t * bit_offset);

int64_t split(const pngenc_image_desc * desc, uint8_t * dst);
pngenc_result write_png(const pngenc_image_desc * desc, const char * file_name);
