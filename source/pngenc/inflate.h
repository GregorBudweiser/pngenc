#pragma once
#include <stdint.h>
#include "huffman.h"

typedef struct _deflate_codec {
    huffman_codec huff;
    uint64_t bit_offset;
} deflate_codec;

typedef struct _pngenc_decoder* pngenc_decoder;

int64_t read_dynamic_header(const uint8_t * src, uint32_t src_size,
                            deflate_codec * deflate);
int decode_data_full(deflate_codec *deflate,
                     const uint8_t * src, uint32_t src_size,
                     uint8_t *dst, uint32_t dst_size);

uint8_t get_num_extra_bits(uint32_t symbol_idx);
uint32_t base_value(uint8_t num_extra_bits);
uint32_t get_length(uint32_t extra_bits, uint8_t num_extra_bits,
                    uint32_t symbol_idx);

uint8_t get_num_extra_bits_dist(uint32_t symbol_idx);
uint32_t base_value_dist(uint8_t num_extra_bits);
uint32_t get_dist(uint32_t extra_bits, uint8_t num_extra_bits,
                  uint32_t symbol_idx);
