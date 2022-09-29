#pragma once
#include <stdint.h>

int64_t write_deflate_block_huff_only(uint8_t * dst, const uint8_t * src,
                                       uint32_t num_bytes, uint8_t last_block);

int64_t write_deflate_block_uncompressed(uint8_t * dst, const uint8_t * src,
                                         uint32_t num_bytes, uint8_t last_block);

int64_t write_deflate_block_rle(uint8_t * dst, const uint8_t * src,
                                   uint32_t num_bytes, uint8_t last_block);
