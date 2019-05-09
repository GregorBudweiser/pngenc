#pragma once

#include "pngenc.h"

uint16_t swap_uint16(uint16_t val);

void push_bits(uint64_t bits, uint64_t nbits, uint8_t * data,
               uint64_t * bit_offset);

int split(const pngenc_encoder encoder, const pngenc_image_desc * desc,
          pngenc_user_write_callback callback, void * user_data);

pngenc_result write_png(const pngenc_encoder encoder,
                        const pngenc_image_desc * desc,
                        pngenc_user_write_callback callback, void *user_data);
