#include "node_deflate.h"
#include "adler32.h"
#include "huffman.h"
#include "matcher.h"
#include "utils.h"
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <math.h>

int64_t write_deflate_uncompressed(struct _pngenc_node * node,
                                   const uint8_t * data, uint32_t size);
int64_t write_deflate_compressed(struct _pngenc_node * node,
                                 const uint8_t * data, uint32_t size);
int64_t finish_deflate_uncompressed(struct _pngenc_node * node);
int64_t finish_deflate_compressed(struct _pngenc_node * node);
int64_t init_deflate(struct _pngenc_node * node);

/**
 * Creates the deflate stream for the given node.
 */
int64_t deflate_encode(pngenc_node_deflate * node, uint8_t last_block);

/**
 * Creates a deflate header with dynamic huffman encoding.
 * This is the "huffman only" version. No length/distance codes are handled.
 *
 * @see dynamic_huffman_header_full()
 */
int64_t dynamic_huffman_header(pngenc_node_deflate * node,
                               uint64_t *bit_offset);

int64_t dynamic_huffman_header_full(pngenc_node_deflate * node,
                                    uint64_t *bit_offset);

int node_deflate_init(pngenc_node_deflate * node,
                      pngenc_compression_strategy strategy) {
    node->strategy = strategy;
    node->base.buf_size = strategy == PNGENC_NO_COMPRESSION
            ? 0xFFFF // Max size in uncompressed case
            : 1024*1024; // Compress 1 MB at a time

    // conservative guess for max size
    uint64_t max_compressed_size = node->base.buf_size*2 + 2048;
    // allocate input and output buffers at once
    node->base.buf = malloc(node->base.buf_size + max_compressed_size);
    node->base.buf_pos = 0;
    node->base.next = 0;

    // add adler checksum
    adler_init(&node->adler);

    node->compressed_buf = (uint8_t*)node->base.buf + node->base.buf_size;
    memset(node->compressed_buf, 0, node->base.buf_size*2 + 2048);
    node->bit_offset = 0;

    // set functions
    node->base.init = &init_deflate;
    node->base.finish = strategy == PNGENC_NO_COMPRESSION
            ? &finish_deflate_uncompressed
            : &finish_deflate_compressed;
    node->base.write = strategy == PNGENC_NO_COMPRESSION
            ? &write_deflate_uncompressed
            : &write_deflate_compressed;

    return PNGENC_SUCCESS;
}

void node_destroy_deflate(pngenc_node_deflate * node) {
    assert(node);

    free(node->base.buf);
}

void push_bits(uint64_t bits, uint64_t nbits, uint8_t * data,
               uint64_t * bit_offset) {
    assert(nbits < 41);

    uint64_t local_offset = (*bit_offset) & 0x7;
    uint8_t * local_data_ptr = data + ((*bit_offset) >> 3);
    uint64_t local_data = *((uint64_t*)local_data_ptr);
    local_data |= (bits << local_offset);
    *((uint64_t*)local_data_ptr) = local_data;
    *bit_offset = (*bit_offset) + nbits;
}

/**
 * consumes data from the input node and runs the checksum on it.
 */
int64_t consume_input(pngenc_node_deflate * node, const uint8_t * data,
                      uint32_t size) {
    uint32_t bytes_copied = min_u32(size, (uint32_t)(node->base.buf_size
                                                   - node->base.buf_pos));
#if __arm__
    memcpy((uint8_t*)node->base.buf + node->base.buf_pos, data, bytes_copied);
    adler_update(&node->adler, (uint8_t*)node->base.buf + node->base.buf_pos,
                 bytes_copied);
#else
    adler_copy_on_update(&node->adler, data, bytes_copied,
                         (uint8_t*)node->base.buf + node->base.buf_pos);
#endif
    node->base.buf_pos += bytes_copied;

    return bytes_copied;
}

/**
 * This function will be called multiple times until all data has been
 * processed.
 */
int64_t write_deflate_compressed(struct _pngenc_node * n, const uint8_t * data,
                                 uint32_t size) {
    // do deflate compression
    pngenc_node_deflate * node = (pngenc_node_deflate*)n;
    if(node->base.buf_pos == node->base.buf_size) { // data ready?
        RETURN_ON_ERROR(deflate_encode(node, 0));
        node->base.buf_pos = 0;
    }

    // prepare data for compression
    return consume_input(node, data, size);
}

int64_t deflate_encode(pngenc_node_deflate * node, uint8_t last_block) {
    uint64_t bit_offset = node->bit_offset;

    // write dynamic block header
    push_bits(last_block, 1, node->compressed_buf, &bit_offset); // last block?
    push_bits(2, 2, node->compressed_buf, &bit_offset); // dyn. huf. tree (10)_2

    int64_t num_bytes = node->strategy == PNGENC_FULL_COMPRESSION
            ? dynamic_huffman_header_full(node, &bit_offset)
            : dynamic_huffman_header(node, &bit_offset);
    if(num_bytes < 0)
        return num_bytes; // Error exit

    bit_offset = bit_offset % 8;
    int64_t copy_last_byte = (bit_offset == 0 || last_block == 1) ? 0 : 1;

    RETURN_ON_ERROR(node_write(node->base.next, node->compressed_buf,
                               num_bytes - copy_last_byte));
    // clear for next block
    memset(node->compressed_buf, 0, num_bytes - copy_last_byte);

    // a zlib block may not be aligned to byte-boundary
    // in this case we have to copy the last byte to the next round..
    if(copy_last_byte) {
        node->compressed_buf[0] = node->compressed_buf[num_bytes-1];
        node->compressed_buf[num_bytes-1] = 0;
    }
    node->bit_offset = bit_offset;

    return PNGENC_SUCCESS;
}

int64_t write_deflate_uncompressed(struct _pngenc_node * n,
                                   const uint8_t * data, uint32_t size) {
    pngenc_node_deflate * node = (pngenc_node_deflate*)n;
    assert(node->base.buf_pos <= node->base.buf_size);

    // buffer full?
    if(node->base.buf_pos == node->base.buf_size) {
        struct zlib_header {
            uint8_t padding;
            uint8_t b_type;
            uint16_t len;
            uint16_t nlen;
        } block = {
            0,
            0,
            (uint16_t)node->base.buf_size,
            ~(uint16_t)node->base.buf_size
        };

        // write another zlib block
        RETURN_ON_ERROR(node_write(node->base.next,
                                   (uint8_t*)&block.b_type, 5));
        RETURN_ON_ERROR(node_write(node->base.next,
                                   node->base.buf, node->base.buf_size));

        node->base.buf_pos = 0;
    }

    // prepare data
    return consume_input(node, data, size);
}

int64_t finish_deflate_compressed(struct _pngenc_node * n) {
    pngenc_node_deflate * node = (pngenc_node_deflate*)n;
    if(node->base.buf_pos > 0) {
        RETURN_ON_ERROR(deflate_encode(node, 1));
        node->base.buf_pos = 0;
    }

    // finally write the checksum
    uint32_t adler_checksum = swap_endianness32(
                adler_get_checksum(&node->adler));
    RETURN_ON_ERROR(node_write(node->base.next, (uint8_t*)&adler_checksum, 4));

    // .. and we're done!
    return PNGENC_SUCCESS;
}

int64_t finish_deflate_uncompressed(struct _pngenc_node * n) {
    pngenc_node_deflate * node = (pngenc_node_deflate*)n;
    if(node->base.buf_pos > 0) {
        struct zlib_header {
            uint8_t padding;
            uint8_t b_type;
            uint16_t len;
            uint16_t nlen;
        } block = {
            0,
            1,
            (uint16_t)node->base.buf_pos,
            ~(uint16_t)node->base.buf_pos
        };

        // write last zlib block
        RETURN_ON_ERROR(node_write(node->base.next,
                                   (uint8_t*)&block.b_type, 5));
        RETURN_ON_ERROR(node_write(node->base.next,
                                   node->base.buf, node->base.buf_pos));

        node->base.buf_pos = 0;
    }

    // finally write the checksum
    uint32_t adler_checksum = swap_endianness32(
                adler_get_checksum(&node->adler));
    RETURN_ON_ERROR(node_write(node->base.next, (uint8_t*)&adler_checksum, 4));

    // .. and we're done!
    return PNGENC_SUCCESS;
}

int64_t init_deflate(struct _pngenc_node * node) {
    // write zlib header
    struct zlib_hdr {
        uint8_t compression_method;
        uint8_t compression_flags;
    } hdr;
    hdr.compression_method = 0x78;  // 7 => 32K window (2^7); 8 => "Deflate"
    hdr.compression_flags = (31 - (((uint32_t)hdr.compression_method*256) % 31))
                          | (0 << 5) | (0 << 6); // FCHECK | FDICT | FLEVEL;
    return node_write(node->next, (uint8_t*)&hdr, sizeof(hdr));
}

int64_t dynamic_huffman_header(pngenc_node_deflate * node,
                               uint64_t * bit_offset) {
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);
    huffman_encoder_add(encoder.histogram, node->base.buf,
                        (uint32_t)node->base.buf_pos);
    encoder.histogram[256] = 1; // terminator

    // in deflate the huffman codes are limited to 15 bits
    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder, 15, 0.8));
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&encoder));

    huffman_encoder code_length_encoder;
    huffman_encoder_init(&code_length_encoder);
    huffman_encoder_add(code_length_encoder.histogram,
                        encoder.code_lengths, 257);
    // we need to write the zero length for the distance code
    code_length_encoder.histogram[0]++;

    // code length codes limited to 7 bits
    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&code_length_encoder, 7, 0.8));
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

    uint8_t * data = node->compressed_buf;
    push_bits(HLIT,  5, data, bit_offset);
    push_bits(HDIST, 5, data, bit_offset);
    push_bits(HCLEN, 4, data, bit_offset);

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
    push_bits(0, 3, data, bit_offset); // no length codes
    push_bits(0, 3, data, bit_offset);
    push_bits(0, 3, data, bit_offset);

    push_bits(code_length_encoder.code_lengths[ 0], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 8], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 7], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 9], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 6], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[10], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 5], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[11], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 4], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[12], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 3], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[13], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 2], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[14], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 1], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[15], 3, data, bit_offset);

    /*
     *        HLIT + 257 code lengths for the literal/length alphabet,
     *           encoded using the code length Huffman code
     */
    uint32_t i;
    for(i = 0; i < 257; i++) {
        push_bits(code_length_encoder.symbols[encoder.code_lengths[i]],
                  code_length_encoder.code_lengths[encoder.code_lengths[i]],
                  data, bit_offset);
    }

    /*
     *        HDIST + 1 code lengths for the distance alphabet,
     *           encoded using the code length Huffman code
     */
    push_bits(code_length_encoder.symbols[0],
              code_length_encoder.code_lengths[0], data, bit_offset);

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
    RETURN_ON_ERROR(huffman_encoder_encode(&encoder, node->base.buf,
                                           (uint32_t)node->base.buf_pos, data,
                                           bit_offset));
    push_bits(encoder.symbols[256], encoder.code_lengths[256],
              data, bit_offset);

    return ((*bit_offset)+7)/8;
}

int64_t dynamic_huffman_header_full(pngenc_node_deflate * node,
                                    uint64_t * bit_offset) {
    huffman_encoder encoder; // literal alphabet
    huffman_encoder distance_encoder; // distance codes
    huffman_encoder code_length_encoder; // code lengths
    huffman_encoder_init(&encoder);
    huffman_encoder_init(&distance_encoder);
    huffman_encoder_init(&code_length_encoder);

    // TODO: put in node
    //uint16_t out[100];
    uint16_t * out = (uint16_t*)malloc(sizeof(uint16_t)*node->base.buf_pos);
    memset(out, 0, sizeof(uint16_t)*node->base.buf_pos);
    uint32_t out_length;

    // histogramming + matching
    if(node->base.buf_pos > 0xFFFFFFFFULL) {
        return PNGENC_ERROR; // TODO: Handle larger inputs
    }
    histogram(node->base.buf, (uint32_t)node->base.buf_pos, encoder.histogram,
              distance_encoder.histogram, out, &out_length);
    encoder.histogram[256] = 1; // terminator

    // Workaround for the case that there is only one dist. symbol
    // (at least two symbols are needed for a valid huffman-tree (?))
    // TODO: Fix this
    distance_encoder.histogram[0]++;
    distance_encoder.histogram[1]++;

    // in deflate the huffman codes are limited to 15 bits
    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder, 15, 0.95));
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&encoder));
    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&distance_encoder, 15, 0.95));
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&distance_encoder));

    // add code lengths of literals and distances to code-length histogram
    huffman_encoder_add(code_length_encoder.histogram,
                        encoder.code_lengths, HUFF_MAX_SIZE);
    huffman_encoder_add(code_length_encoder.histogram,
                        distance_encoder.code_lengths, 30);
    // we need to write the zero length for the distance code
    code_length_encoder.histogram[0]++;

    // code length codes limited to 7 bits
    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&code_length_encoder, 7, 0.95));
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&code_length_encoder));

    //huffman_encoder_print(&code_length_encoder, "code_lengths");
    //huffman_encoder_print(&distance_encoder, "distances");
    //huffman_encoder_print(&encoder, "literals");

    /*
     * From the RFC:
     *
     * We can now define the format of the block:
     *
     *               5 Bits: HLIT,  # of Literal/Length codes - 257 (257 - 286)
     *               5 Bits: HDIST, # of Distance codes - 1        (1 - 32)
     *               4 Bits: HCLEN, # of Code Length codes - 4     (4 - 19)
     */
    // 256 literals + 1 termination symbol
    const uint32_t HLIT = HUFF_MAX_SIZE - 257;
    // 30 distance codes; set it to 0 to signal that we only use literals
    const uint32_t HDIST = 29 - 1; // @TODO: What's the exact limit and why?
    // 19 code lengths of the actual code lengths (SEE RFC1951)
    const uint32_t HCLEN = 19 - 4;

    // Start outputting stuff
    uint8_t * data = node->compressed_buf;
    push_bits(HLIT,  5, data, bit_offset);
    push_bits(HDIST, 5, data, bit_offset);
    push_bits(HCLEN, 4, data, bit_offset);

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
    // 19 code length lengths..
    push_bits(0, 3, data, bit_offset); // no length codes for code lengths :)
    push_bits(0, 3, data, bit_offset);
    push_bits(0, 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 0], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 8], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 7], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 9], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 6], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[10], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 5], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[11], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 4], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[12], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 3], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[13], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 2], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[14], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[ 1], 3, data, bit_offset);
    push_bits(code_length_encoder.code_lengths[15], 3, data, bit_offset);

    /*
     *        HLIT + 257 code lengths for the literal/length alphabet,
     *           encoded using the code length Huffman code
     */
    uint32_t i;
    for(i = 0; i < HLIT + 257; i++) {
        push_bits(code_length_encoder.symbols[encoder.code_lengths[i]],
                  code_length_encoder.code_lengths[encoder.code_lengths[i]],
                  data, bit_offset);
    }

    /*
     *        HDIST + 1 code lengths for the distance alphabet,
     *           encoded using the code length Huffman code
     */
    for(i = 0; i < HDIST+1; i++) {
        push_bits(code_length_encoder.symbols[distance_encoder.code_lengths[i]],
                  code_length_encoder.code_lengths[distance_encoder.code_lengths[i]],
                  data, bit_offset);
    }

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
    // Temporary buffer to final compressed format
    RETURN_ON_ERROR(huffman_encoder_encode_full_simple(&encoder, &distance_encoder,
                                                       out, out_length,
                                                       data, bit_offset));

    //printf("Data to compress: %dkB\n", (int)(node->base.buf_pos/1000));
    //printf("Compressed to: %dB\n", (int)(out_length/8));

    // terminator symbol / end of stream
    push_bits(encoder.symbols[256], encoder.code_lengths[256],
              data, bit_offset);

    return ((*bit_offset)+7)/8;
}
