#include "deflate.h"
#include "huffman.h"
#include "utils.h"
#include <string.h>

int64_t write_deflate_block_compressed(uint8_t * dst, const uint8_t * src,
                                       uint32_t num_bytes, uint8_t last_block) {
    // clear dst memory for header and first four bytes of stream
    for (int i = 0; i < 287+4; i++) {
        dst[i] = 0;
    }

    // zlib header
    uint64_t bit_offset = 0;

    // write dynamic block header
    push_bits(0, 1, dst, &bit_offset); // not last block
    push_bits(2, 2, dst, &bit_offset); // compressed block: dyn. huff. (10)_2

    // deflate
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);
    huffman_encoder_add(encoder.histogram, src, num_bytes);
    encoder.histogram[256] = 1; // terminator

    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder, 15, POW_95));
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&encoder));

    huffman_encoder code_length_encoder;
    huffman_encoder_init(&code_length_encoder);
    huffman_encoder_add(code_length_encoder.histogram,
                        encoder.code_lengths, 257);
    // we need to write the zero length for the distance code
    code_length_encoder.histogram[0]++;

    // code length codes limited to 7 bits
    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&code_length_encoder, 7, POW_80));
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&code_length_encoder));

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

    push_bits(HLIT,  5, dst, &bit_offset);
    push_bits(HDIST, 5, dst, &bit_offset);
    push_bits(HCLEN, 4, dst, &bit_offset);

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
    push_bits(0, 3, dst, &bit_offset); // no length codes
    push_bits(0, 3, dst, &bit_offset);
    push_bits(0, 3, dst, &bit_offset);

    push_bits(code_length_encoder.code_lengths[ 0], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[ 8], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[ 7], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[ 9], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[ 6], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[10], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[ 5], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[11], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[ 4], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[12], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[ 3], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[13], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[ 2], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[14], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[ 1], 3, dst, &bit_offset);
    push_bits(code_length_encoder.code_lengths[15], 3, dst, &bit_offset);

    /*
     *        HLIT + 257 code lengths for the literal/length alphabet,
     *           encoded using the code length Huffman code
     */
    uint32_t i;
    for(i = 0; i < 257; i++) {
        push_bits(code_length_encoder.symbols[encoder.code_lengths[i]],
                  code_length_encoder.code_lengths[encoder.code_lengths[i]],
                  dst, &bit_offset);
    }

    /*
     *        HDIST + 1 code lengths for the distance alphabet,
     *           encoded using the code length Huffman code
     */
    push_bits(code_length_encoder.symbols[0],
              code_length_encoder.code_lengths[0], dst, &bit_offset);

    /*
     *        The actual compressed data of the block,
     *           encoded using the literal/length and distance Huffman
     *           codes
     *
     *        The literal/length symbol 256 (end of data),
     *           encoded using the literal/length Huffman code
     *
     *  The code length repeat codes can cross from HLIT + 257 to the
     *  HDIST + 1 code lengths.  In other words, all code lengths form
     *  a single sequence of HLIT + HDIST + 258 values.
     */
    RETURN_ON_ERROR(huffman_encoder_encode(&encoder, src, num_bytes, dst,
                                           &bit_offset));
    // Terminator symbol (i.e. compressed block ends here)
    push_bits(encoder.symbols[256], encoder.code_lengths[256],
              dst, &bit_offset);

    // zflush with uncompressed block to achieve byte-alignment
    push_bits(last_block, 1, dst, &bit_offset); // last block?
    push_bits(0, 2, dst, &bit_offset);          // uncompressed block
    uint64_t encoded_bytes = (bit_offset + 7) / 8;
    dst += encoded_bytes;
    *dst++ = 0;
    *dst++ = 0;
    *dst++ = 0xFF;
    *dst++ = 0xFF;

    return (int64_t)encoded_bytes + 4;
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
