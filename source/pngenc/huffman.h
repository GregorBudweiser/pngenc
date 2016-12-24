#pragma once
#include <stdint.h>

#define HUFF_MAX_SIZE 285

typedef struct _huffman_encoder {
    uint32_t histogram[HUFF_MAX_SIZE];
    uint16_t symbols[HUFF_MAX_SIZE];     // max. 15 bit to represent value
    uint8_t code_lengths[HUFF_MAX_SIZE]; // max.  4 bit to represent length
} huffman_encoder;


void huffman_encoder_init(huffman_encoder * encoder);
int huffman_encoder_add(huffman_encoder * encoder, const uint8_t * symbols,
                        uint32_t length);
int huffman_encoder_add_simple(huffman_encoder * encoder, const uint8_t * data,
                               uint32_t length);
int huffman_encoder_add_single(huffman_encoder * encoder, uint8_t data);
int huffman_encoder_build_tree(huffman_encoder * encoder);
int huffman_encoder_build_tree_limited(huffman_encoder * encoder, uint8_t limit);
int huffman_encoder_encode(const huffman_encoder * encoder, const uint8_t * src,
                           uint32_t length, uint8_t * dst, uint64_t *offset);
int huffman_encoder_encode_simple(const huffman_encoder * encoder,
                                  const uint8_t * src, uint32_t length,
                                  uint8_t * dst, uint64_t * offset);
int huffman_encoder_encode_full_simple(const huffman_encoder * encoder_hist,
                                       const huffman_encoder * encoder_dist,
                                       const uint16_t *src, uint32_t length,
                                       uint8_t * dst, uint64_t * offset);
int huffman_encoder_get_max_length(const huffman_encoder * encoder);
int huffman_encoder_build_codes_from_lengths(huffman_encoder * encoder);
