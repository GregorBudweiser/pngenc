#pragma once
#include <stdint.h>

typedef struct _huffman_encoder {
    uint32_t histogram[257];
    uint16_t symbols[257];     // max. 15 bit to represent value
    uint8_t code_lengths[257]; // max.  4 bit to represent length
} huffman_encoder;


void huffman_encoder_init(huffman_encoder * encoder);
int huffman_encoder_add(huffman_encoder * encoder, const uint8_t * symbols,
                        uint32_t length);
int huffman_encoder_add_simple(huffman_encoder * encoder, const uint8_t * data,
                               uint32_t length);
int huffman_encoder_add_single(huffman_encoder * encoder, uint8_t data);
int huffman_encoder_build_tree(huffman_encoder * encoder);
int huffman_encoder_encode(const huffman_encoder * encoder, const uint8_t * src,
                           uint32_t length, uint8_t * dst, uint64_t *offset);
int huffman_encoder_encode_simple(const huffman_encoder * encoder,
                                  const uint8_t * src, uint32_t length,
                                  uint8_t * dst, uint64_t * offset);
int huffman_encoder_get_max_length(const huffman_encoder * encoder);
int huffman_encoder_build_codes_from_lengths(huffman_encoder * encoder);
