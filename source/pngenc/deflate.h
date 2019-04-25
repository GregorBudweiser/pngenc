#pragma once
#include <stdint.h>

void push_bits(uint64_t bits, uint64_t nbits, uint8_t * data,
               uint64_t * bit_offset);

int64_t write_deflate_block_compressed(uint8_t * dst, const uint8_t * src,
                                       uint32_t num_bytes, uint8_t last_block);

int64_t write_deflate_block_uncompressed(uint8_t * dst, const uint8_t * src,
                                         uint32_t num_bytes);

