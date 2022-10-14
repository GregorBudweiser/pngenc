#pragma once
#include <stdint.h>
#include "pow.h"

#define HUFF_MAX_SIZE 288

typedef struct _huffman_encoder {
    uint32_t histogram[HUFF_MAX_SIZE];
    uint16_t symbols[HUFF_MAX_SIZE];     // max. 15 bit to represent value
    uint8_t code_lengths[HUFF_MAX_SIZE]; // max.  4 bit to represent length
} huffman_encoder;

void huffman_encoder_init(huffman_encoder * encoder);

void huffman_encoder_add(uint32_t *histogram, const uint8_t * symbols,
                         uint32_t length);
void huffman_encoder_add32(uint32_t *histogram, const uint8_t * symbols,
                           uint32_t length);
void huffman_encoder_add64(uint32_t *histogram, const uint8_t * symbols,
                           uint32_t length);
void huffman_encoder_add64_2(uint32_t *histogram, const uint8_t * symbols,
                             uint32_t length);
void huffman_encoder_add_simple(uint32_t * histogram, const uint8_t * data,
                                uint32_t length);

void huffman_encoder_add_rle_simple(uint32_t *histogram, const uint8_t * symbols,
                                    uint32_t length);
void huffman_encoder_add_rle(uint32_t *histogram, const uint8_t * symbols,
                             uint32_t length);

void huffman_encoder_build_tree(huffman_encoder * encoder);
void huffman_encoder_build_tree_limited(huffman_encoder * encoder, uint8_t limit,
                                        power_coefficient power);

/**
 * Encode data using "huffman only" strategy.
 *
 * @param encoder: huffman encoder
 * @param src: data to encode
 * @param length: number of bytes to encode
 * @param dst: Output buffer. May be uninitialized past given byte-aligned bit-offset
 * @param offset: offset into 'dst' in bits
 */
void huffman_encoder_encode(const huffman_encoder * encoder, const uint8_t * src,
                            uint32_t length, uint8_t * dst, uint64_t *offset);

/**
 * Encode "huffman only" with aligned writes.
 */
void huffman_encoder_encode64_3(const huffman_encoder * encoder, const uint8_t * src,
                                uint32_t length, uint8_t * dst, uint64_t *offset);

/**
 * Encode "huffman only" with unaligned writes.
 */
void huffman_encoder_encode64_4(const huffman_encoder * encoder, const uint8_t * src,
                                uint32_t length, uint8_t * dst, uint64_t *offset);

/**
 * Reference implementation for "huffman only"
 */
void huffman_encoder_encode_simple(const huffman_encoder * encoder,
                                   const uint8_t * src, uint32_t length,
                                   uint8_t * dst, uint64_t * offset);

/**
 * Reference implementation for "RLE"
 */
void huffman_encoder_encode_rle_simple(const huffman_encoder * encoder,
                                       const huffman_encoder *dist_encoder,
                                       const uint8_t * src, uint32_t length,
                                       uint8_t * dst, uint64_t * offset);

/**
 * Encode using "RLE" strategy.
 */
void huffman_encoder_encode_rle(const huffman_encoder * encoder,
                                const huffman_encoder *dist_encoder,
                                const uint8_t * src, uint32_t length,
                                uint8_t * dst, uint64_t * offset);

uint32_t huffman_encoder_get_max_length(const huffman_encoder * encoder);
void huffman_encoder_build_codes_from_lengths(huffman_encoder * encoder);
uint32_t huffman_encoder_get_num_literals(const huffman_encoder * encoder);

void huffman_encoder_print(const huffman_encoder * encoder, const char * name);

void push_bits(uint64_t bits, uint8_t nbits, uint8_t * data,
               uint64_t * bit_offset);
uint64_t push_bits_a(uint64_t bits, uint8_t nbits, uint8_t * dst,
                     uint64_t bit_offset);
uint64_t push_bits_u(uint64_t bits, uint8_t nbits, uint8_t * dst,
                     uint64_t bit_offset);
