#include "deflate.h"
#include "huffman.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

void write_header_huff_only(uint8_t * dst, uint64_t * bit_offset,
                            huffman_encoder * encoder, int last_block) {
    // write dynamic block header
    push_bits(last_block, 1, dst, bit_offset); // last block?
    push_bits(2, 2, dst, bit_offset); // compressed block: dyn. huff. (10)_2

    encoder->histogram[256] = 1; // terminator
    huffman_encoder_build_tree_limited(encoder, 15, POW_95);
    huffman_encoder_build_codes_from_lengths(encoder);

    huffman_encoder code_length_encoder;
    huffman_encoder_init(&code_length_encoder);
    huffman_encoder_add(code_length_encoder.histogram,
                        encoder->code_lengths, 257);
    // we need to write the zero length for the distance code
    code_length_encoder.histogram[0]++;

    // code length codes limited to 7 bits
    huffman_encoder_build_tree_limited(&code_length_encoder, 7, POW_80);
    huffman_encoder_build_codes_from_lengths(&code_length_encoder);

    /*
     * From the RFC:
     *
     * We can now define the format of the block:
     *
     *               5 Bits: HLIT, # of Literal/Length codes - 257 (257 - 286)
     *               5 Bits: HDIST, # of Distance codes - 1        (1 - 32)
     *               4 Bits: HCLEN, # of Code Length codes - 4     (4 - 19)
     */
    // 256 literals + 1 termination symbol
    const uint8_t HLIT = 0;
    // 1 distance code; set it to 0 to signal that we only use literals
    const uint8_t HDIST = 0;
    // 19 code lengths of the actual code lengths (SEE RFC1951)
    const uint8_t HCLEN = 0xF;

    push_bits(HLIT,  5, dst, bit_offset);
    push_bits(HDIST, 5, dst, bit_offset);
    push_bits(HCLEN, 4, dst, bit_offset);

    /*
     *       (HCLEN + 4) x 3 bits: code lengths for the code length
     *           alphabet given just above, in the order: 16, 17, 18,
     *           0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
     *
     *           These code lengths are interpreted as 3-bit integers
     *           (0-7); as above, a code length of 0 means the
     *           corresponding symbol (literal/length or distance code
     *           length) is not used.
     */
    push_bits(0, 3, dst, bit_offset); // no length codes
    push_bits(0, 3, dst, bit_offset);
    push_bits(0, 3, dst, bit_offset);

    push_bits(code_length_encoder.code_lengths[ 0], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 8], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 7], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 9], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 6], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[10], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 5], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[11], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 4], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[12], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 3], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[13], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 2], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[14], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 1], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[15], 3, dst, bit_offset);

    /*
     *        HLIT + 257 code lengths for the literal/length alphabet,
     *           encoded using the code length Huffman code
     */
    uint32_t i;
    for(i = 0; i < HLIT + 257; i++) {
        push_bits(code_length_encoder.symbols[encoder->code_lengths[i]],
                  code_length_encoder.code_lengths[encoder->code_lengths[i]],
                  dst, bit_offset);
    }

    /*
     *        HDIST + 1 code lengths for the distance alphabet,
     *           encoded using the code length Huffman code
     */
    push_bits(code_length_encoder.symbols[0],
              code_length_encoder.code_lengths[0], dst, bit_offset);
}

int64_t write_deflate_block_huff_only(uint8_t * dst, const uint8_t * src,
                                       uint32_t num_bytes, uint8_t last_block) {
    // clear dst memory for header and first four bytes of stream
    for (int i = 0; i < 287+4; i++) {
        dst[i] = 0;
    }

    // zlib header
    uint64_t bit_offset = 0;

    // deflate
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);
    huffman_encoder_add(encoder.histogram, src, num_bytes);
    encoder.histogram[256] = 1; // terminator

    write_header_huff_only(dst, &bit_offset, &encoder, last_block);
    huffman_encoder_encode(&encoder, src, num_bytes, dst, &bit_offset);
    // Terminator symbol (i.e. compressed block ends here)
    push_bits(encoder.symbols[256], encoder.code_lengths[256],
              dst, &bit_offset);

    if (!last_block) {
        // zflush with uncompressed block to achieve byte-alignment
        push_bits(0, 3, dst, &bit_offset); // not last, uncompressed block
        uint64_t encoded_bytes = (bit_offset + 7) / 8;
        dst += encoded_bytes;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0xFF;
        *dst++ = 0xFF;
        return (int64_t)encoded_bytes + 4;
    } else {
        return (bit_offset + 7) / 8;
    }
}

void write_header_fixed(uint8_t * dst, uint64_t * bit_offset,
                        huffman_encoder * encoder,
                        huffman_encoder * dist_encoder) {
    // write dynamic block header
    push_bits(0, 1, dst, bit_offset); // not last block
    push_bits(1, 2, dst, bit_offset); // compressed block: static huff.

    // literals; Set lengths for fixed tree (as per RFC1951)
    huffman_encoder_init(encoder);
    for(uint32_t i = 0; i <= 143; i++) {
        encoder->code_lengths[i] = 8;
    }
    for(uint32_t i = 144; i <= 255; i++) {
        encoder->code_lengths[i] = 9;
    }
    for(uint32_t i = 256; i <= 279; i++) {
        encoder->code_lengths[i] = 7;
    }
    for(uint32_t i = 280; i <= 287; i++) {
        encoder->code_lengths[i] = 8;
    }
    huffman_encoder_build_codes_from_lengths(encoder);

    // distances
    huffman_encoder_init(dist_encoder);
    for(uint32_t i = 0; i <= 29; i++) {
        dist_encoder->code_lengths[i] = 5;
    }
    huffman_encoder_build_codes_from_lengths(dist_encoder);
}

void write_header_rle(uint8_t * dst, uint64_t * bit_offset,
                      huffman_encoder * encoder,
                      huffman_encoder * dist_encoder,
                      int last_block) {
    // clear dst memory for header and first four bytes of stream
    for (int i = 0; i < 287+4; i++) {
        dst[i] = 0;
    }

    // write dynamic block header
    push_bits(last_block, 1, dst, bit_offset); // last block?
    push_bits(2, 2, dst, bit_offset); // compressed block: dyn. huff. (10)_2

    encoder->histogram[256] = 1; // terminator
    huffman_encoder_build_tree_limited(encoder, 15, POW_95);
    huffman_encoder_build_codes_from_lengths(encoder);

    huffman_encoder_build_tree_limited(dist_encoder, 15, POW_95);
    huffman_encoder_build_codes_from_lengths(dist_encoder);

    /*
     * From the RFC:
     *
     * We can now define the format of the block:
     *
     *               5 Bits: HLIT, # of Literal/Length codes - 257 (257 - 286)
     *               5 Bits: HDIST, # of Distance codes - 1        (1 - 32)
     *               4 Bits: HCLEN, # of Code Length codes - 4     (4 - 19)
     */
    // all 286 literals/lengths
    const uint8_t HLIT = 0x1D;
    // 1 distance code for backward distance 1
    const uint8_t HDIST = 1;
    // 19 code lengths of the actual code lengths (SEE RFC1951)
    const uint8_t HCLEN = 0xF;

    huffman_encoder code_length_encoder;
    huffman_encoder_init(&code_length_encoder);
    huffman_encoder_add(code_length_encoder.histogram,
                        encoder->code_lengths, HLIT + 257);
    // we need to write the zero length for the distance codes plus the one for distance 1
    code_length_encoder.histogram[dist_encoder->code_lengths[0]]++;
    code_length_encoder.histogram[dist_encoder->code_lengths[1]]++;

    // code length codes limited to 7 bits
    huffman_encoder_build_tree_limited(&code_length_encoder, 7, POW_80);
    huffman_encoder_build_codes_from_lengths(&code_length_encoder);

    push_bits(HLIT,  5, dst, bit_offset);
    push_bits(HDIST, 5, dst, bit_offset);
    push_bits(HCLEN, 4, dst, bit_offset);

    /*
     *       (HCLEN + 4) x 3 bits: code lengths for the code length
     *           alphabet given just above, in the order: 16, 17, 18,
     *           0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
     *
     *           These code lengths are interpreted as 3-bit integers
     *           (0-7); as above, a code length of 0 means the
     *           corresponding symbol (literal/length or distance code
     *           length) is not used.
     */
    push_bits(0, 3, dst, bit_offset); // no length codes
    push_bits(0, 3, dst, bit_offset);
    push_bits(0, 3, dst, bit_offset);

    push_bits(code_length_encoder.code_lengths[ 0], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 8], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 7], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 9], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 6], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[10], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 5], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[11], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 4], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[12], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 3], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[13], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 2], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[14], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 1], 3, dst, bit_offset);
    push_bits(code_length_encoder.code_lengths[15], 3, dst, bit_offset);

    /*
     *        HLIT + 257 code lengths for the literal/length alphabet,
     *           encoded using the code length Huffman code
     */
    for(uint32_t i = 0; i < HLIT + 257; i++) {
        push_bits(code_length_encoder.symbols[encoder->code_lengths[i]],
                  code_length_encoder.code_lengths[encoder->code_lengths[i]],
                  dst, bit_offset);
    }

    /*
     *        HDIST + 1 code lengths for the distance alphabet,
     *           encoded using the code length Huffman code
     */
    for(uint32_t i = 0; i < HDIST + 1; i++) {
        push_bits(code_length_encoder.symbols[dist_encoder->code_lengths[i]],
                  code_length_encoder.code_lengths[dist_encoder->code_lengths[i]],
                  dst, bit_offset);
    }
}

int64_t write_deflate_block_rle(uint8_t * dst, const uint8_t * src,
                                uint32_t num_bytes, uint8_t last_block) {
    // clear dst memory for header and first four bytes of stream
    for (int i = 0; i < 287+4; i++) {
        dst[i] = 0;
    }

    // distances
    huffman_encoder dist_encoder;
    huffman_encoder_init(&dist_encoder);
    dist_encoder.code_lengths[0] = 1;
    dist_encoder.code_lengths[1] = 1;

    // literals; Set lengths for fixed tree (as per RFC1951)
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);
    huffman_encoder_add_rle_approx(encoder.histogram, src, num_bytes);

    uint64_t bit_offset = 0;
    write_header_rle(dst, &bit_offset, &encoder, &dist_encoder, last_block);
    huffman_encoder_encode_rle(&encoder, &dist_encoder, src, num_bytes, dst, &bit_offset);

    // Terminator symbol (i.e. compressed block ends here)
    push_bits(encoder.symbols[256], encoder.code_lengths[256],
              dst, &bit_offset);

    if (!last_block) {
        // zflush with uncompressed block to achieve byte-alignment
        push_bits(0, 3, dst, &bit_offset); // not last, uncompressed block
        uint64_t encoded_bytes = (bit_offset + 7) / 8;
        dst += encoded_bytes;
        *dst++ = 0;
        *dst++ = 0;
        *dst++ = 0xFF;
        *dst++ = 0xFF;
        return (int64_t)encoded_bytes+4;
    } else {
        return (bit_offset + 7) / 8;
    }
}

int64_t write_deflate_block_uncompressed(uint8_t * dst, const uint8_t * src,
                                         uint32_t num_bytes, uint8_t last_block) {
    struct zlib_header {
        uint8_t padding;
        uint8_t b_type;
        uint16_t len;
        uint16_t nlen;
    } hdr;

    const uint8_t * old_dst = dst;

    uint32_t i;
    for(i = 0; i < num_bytes; i += 0xFFFF) {
        uint16_t len = (uint16_t)min_u32(0xFFFF, num_bytes - i);
        hdr.b_type = (last_block && (i + 0xFFFF) >= num_bytes) ? 1 : 0;
        hdr.len = len;
        hdr.nlen = ~len;
        memcpy(dst, &hdr.b_type, 5); // write/cpy header
        dst += 5;
        memcpy(dst, src, len);
        dst += len;
        src += len;
    }

    return dst - old_dst;
}
