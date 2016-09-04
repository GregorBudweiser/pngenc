#include "node_deflate.h"
#include "adler32.h"
#include "huffman.h"
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

int64_t deflate_encode(pngenc_node_deflate * node, uint8_t last_block);
int64_t dynamic_huffman_header(pngenc_node_deflate * node,
                               huffman_encoder * encoder,
                               uint64_t *bit_offset);


int node_deflate_init(pngenc_node_deflate * node,
                         const pngenc_image_desc * image) {
    node->base.buf_size = image->strategy == PNGENC_NO_COMPRESSION
            ? 0xFFFF // Max size in uncompressed case
            : 1024*1024; // Compress 1 MB at a time
    node->base.buf = malloc(node->base.buf_size);
    node->base.buf_pos = 0;
    node->base.next = 0;

    // add adler checksum
    adler_init(&node->adler);
    // conservative guess for max size
    node->compressed_buf = (uint8_t*)malloc(node->base.buf_size*2 + 2048);
    memset(node->compressed_buf, 0, node->base.buf_size*2 + 2048);
    node->bit_offset = 0;

    // set functions
    node->base.init = &init_deflate;
    node->base.finish = image->strategy == PNGENC_NO_COMPRESSION
            ? &finish_deflate_uncompressed
            : &finish_deflate_compressed;
    node->base.write = image->strategy == PNGENC_NO_COMPRESSION
            ? &write_deflate_uncompressed
            : &write_deflate_compressed;

    return PNGENC_SUCCESS;
}

void node_destroy_deflate(pngenc_node_deflate * node) {
    assert(node);

    free(node->base.buf);
    free(node->compressed_buf);
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

int64_t write_deflate_compressed(struct _pngenc_node * n, const uint8_t * data,
                                 uint32_t size) {
    pngenc_node_deflate * node = (pngenc_node_deflate*)n;
    if(node->base.buf_pos == node->base.buf_size) {
        RETURN_ON_ERROR(deflate_encode(node, 0));
        node->base.buf_pos = 0;
    }

    // TODO: Make this block a function..
    uint32_t bytes_copied = min_u32(size, node->base.buf_size
                                        - node->base.buf_pos);
    memcpy(node->base.buf + node->base.buf_pos, data, bytes_copied);
    adler_update(&node->adler, node->base.buf + node->base.buf_pos,
                 bytes_copied);
    node->base.buf_pos += bytes_copied;

    return bytes_copied;
}

int64_t deflate_encode(pngenc_node_deflate * node, uint8_t last_block) {
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);
    uint64_t bit_offset = node->bit_offset;

    // write dynamic block header
    push_bits(last_block, 1, node->compressed_buf, &bit_offset); // last block?
    push_bits(2, 2, node->compressed_buf, &bit_offset); // dyn. huf. tree (10)_2

    int64_t num_bytes = dynamic_huffman_header(node, &encoder, &bit_offset);
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
        } block = { 0, 0, node->base.buf_size, 0 };

        // write another zlib block
        RETURN_ON_ERROR(node_write(node->base.next,
                                   (uint8_t*)&block.b_type, 5));
        RETURN_ON_ERROR(node_write(node->base.next,
                                   node->base.buf, node->base.buf_size));

        node->base.buf_pos = 0;
    }

    // consume as much of current data as possible
    uint32_t bytes_remaining = node->base.buf_size - node->base.buf_pos;
    uint32_t bytes_written = min_u32(bytes_remaining, size);
    memcpy(node->base.buf + node->base.buf_pos, data, bytes_written);
    adler_update(&node->adler, node->base.buf + node->base.buf_pos,
                 bytes_written);
    node->base.buf_pos += bytes_written;
    return bytes_written;
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
        } block = { 0, 1, node->base.buf_pos, ~node->base.buf_pos };

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
    hdr.compression_method = 0x78;
    hdr.compression_flags = (31 - (((uint32_t)hdr.compression_method*256) % 31))
                          | (0 << 5) | (0 << 6); // FCHECK | FDICT | FLEVEL;
    return node_write(node->next, (uint8_t*)&hdr, sizeof(hdr));
}

int64_t dynamic_huffman_header(pngenc_node_deflate * node,
                               huffman_encoder * encoder,
                               uint64_t * bit_offset) {
    RETURN_ON_ERROR(huffman_encoder_add(encoder, node->base.buf,
                                        node->base.buf_pos));
    encoder->histogram[256] = 1; // terminator

    // codes limited to 15 bits
    // TODO: Compute optimal tree
    do {
        RETURN_ON_ERROR(huffman_encoder_build_tree(encoder));
        for(int i = 0; i < 257; i++) {
            if(encoder->histogram[i]) {
                // nonlearly reduce weight of nodes to lower max tree depth
                uint32_t reduced = (uint32_t)pow(encoder->histogram[i], 0.8);
                encoder->histogram[i] = max_u32(1, reduced);
            }
        }
    } while(huffman_encoder_get_max_length(encoder) > 15);
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(encoder));

    huffman_encoder code_length_encoder;
    huffman_encoder_init(&code_length_encoder);
    huffman_encoder_add(&code_length_encoder, encoder->code_lengths, 257);
    // we need to write the zero length for the distance code
    code_length_encoder.histogram[0]++;

    // code length codes limited to 7 bits
    // TODO: Compute optimal tree
    do {
        RETURN_ON_ERROR(huffman_encoder_build_tree(&code_length_encoder));
        for(int i = 0; i < 257; i++) {
            if(code_length_encoder.histogram[i]) {
                // nonlearly reduce weight of nodes to lower max tree depth
                uint32_t reduced =
                        (uint32_t)pow(code_length_encoder.histogram[i], 0.8);
                code_length_encoder.histogram[i] = max_u32(1, reduced);
            }
        }
    } while(huffman_encoder_get_max_length(&code_length_encoder) > 7);
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(
                        &code_length_encoder));

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
        push_bits(code_length_encoder.symbols[encoder->code_lengths[i]],
                  code_length_encoder.code_lengths[encoder->code_lengths[i]],
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
    RETURN_ON_ERROR(huffman_encoder_encode(encoder, node->base.buf,
                                           node->base.buf_pos, data,
                                           bit_offset));
    push_bits(encoder->symbols[256], encoder->code_lengths[256],
              data, bit_offset);

    return ((*bit_offset)+7)/8;
}
