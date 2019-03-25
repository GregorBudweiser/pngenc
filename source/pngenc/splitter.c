#include "splitter.h"
#include "adler32.h"
#include "huffman.h"
#include "crc32.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>


// TODO: Make dynamic
const static uint32_t TARGET_BLOCK_SIZE = 1000000;

static uint8_t my_tmp[1024*1024];
static uint8_t my_buf[1024*1024*10];

/**
 * In case of compression
 */
uint32_t prepare_data(const pngenc_image_desc * image,
                      uint32_t yStart, uint32_t yEnd, uint8_t * dst) {
    // TODO: Check that output is less than 4 GB
    uint64_t y;
    const uint64_t length = image->width * image->num_channels
                          * image->bit_depth / 8;
    for(y = yStart; y < yEnd; y++) {
        *dst++ = 1; // row filter
        if(image->bit_depth == 8) {
            // delta-x row filter
            const uint8_t c = image->num_channels;
            const uint8_t * src = image->data + y*image->row_stride;

            uint64_t i;
            for(i = 0; i < c; i++)
                dst[i] = src[i];

            for(i = c; i < length; i++)
                dst[i] = src[i] - src[i-c];

            dst += length;

        } else { // 16bit
            // delta-x row filter
            const uint64_t c = image->num_channels;
            const uint16_t * src = (const uint16_t*)(image->data + y*image->row_stride);
            uint16_t * dst16 = (uint16_t*)dst;

            uint64_t i;
            for(i = 0; i < c; i++)
                dst16[i] = swap_uint16(src[i]);

            // TODO: Alignment of dst broken becasue of row filter..
            //       This will only work on x86
            for(i = c; i < length/2; i++)
                dst16[i] = swap_uint16(src[i]) - swap_uint16(src[i-c]);

            dst += length;
        }
    }

    return (yEnd - yStart)*(image->width * image->num_channels
                            * image->bit_depth / 8 + 1);
}

int64_t write_deflate_block_compressed(uint8_t * dst, const uint8_t * src,
                                       uint32_t num_bytes, uint8_t last_block) {
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

    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder, 15, 0.95));
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
    push_bits(encoder.symbols[256], encoder.code_lengths[256],
              dst, &bit_offset);

    // zflush with uncompressed block to achieve alignment
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

int64_t process(const pngenc_image_desc * desc, uint32_t yStart,
                uint32_t yEnd, uint8_t * dst, uint8_t * tmp,
                pngenc_adler32 * sum) {
    // prepare data
    uint32_t num_bytes = prepare_data(desc, yStart, yEnd, tmp);

    // update adler
    adler_update(sum, tmp, num_bytes); // <-- this works

    // Idat hdr (part I)
    struct idat_header {
        uint32_t size;
        uint8_t name[4];
    } hdr = {
        0,
        { 'I', 'D', 'A', 'T' }
    };
    uint8_t * const idat_ptr = dst;
    dst += sizeof(hdr); // advance destination pointer

    // compress: zlib-deflate-block, zlib-uncompressed-0-block (zflush)
    uint8_t last_block = yEnd == desc->height ? 1 : 0;
    int64_t result = write_deflate_block_compressed(dst, tmp, num_bytes, last_block);
    if(result < 0)
        return result;
    dst += result;

    if(last_block) {
        uint32_t adler_checksum = swap_endianness32(adler_get_checksum(sum)); // <-- this works
        memcpy(dst, &adler_checksum, 4);
        dst += 4;
        result += 4; // checksum is part of zlib stream
    }

    // Idat hdr (part II)
    hdr.size = swap_endianness32((uint32_t)result);
    memcpy(idat_ptr, &hdr, 8);

    // Idat end/checksum (starts with crc("IDAT"))
    uint32_t crc = crc32c(0xCA50F9E1, idat_ptr+8, (size_t)result);
    crc = swap_endianness32(crc ^ 0xFFFFFFFF);
    memcpy(dst, &crc, 4);
    dst += 4;

    // number of bytes: Idat hdr + data + checksum
    return (int64_t)(dst - idat_ptr);
}

int64_t split(const pngenc_image_desc * desc, uint8_t * dst) {
    const uint8_t * const old_dst = dst;

    // TODO: Assert alignment and size
    uint8_t * tmp = my_tmp; //(uint8_t*)malloc(2*TARGET_BLOCK_SIZE); // pick a safe upper bound

    //uint8_t * dst = (uint8_t*)malloc(2*TARGET_BLOCK_SIZE); // pick a safe upper bound
    //memset(tmp, 0, 2*TARGET_BLOCK_SIZE);

    pngenc_adler32 sum;
    adler_init(&sum);
    uint32_t num_rows = TARGET_BLOCK_SIZE/desc->row_stride + 1;
    uint32_t y;
    int64_t result;
    for(y = 0; y < desc->height; y += num_rows) {
        uint32_t yNext = min_u32(y+num_rows, desc->height);
        memset(tmp, 0, sizeof(my_tmp));
        if((result = process(desc, y, yNext, dst, tmp, &sum)) < 0) {
            return result;
        }
        dst += result;
    }
    //free(tmp);

    // number of bytes
    return dst - old_dst;
}

pngenc_result write_png(const pngenc_image_desc * desc,
                        const char * file_name) {
    FILE * f = fopen(file_name, "wb");
    if(f == 0) {
        return PNGENC_ERROR_FILE_IO;
    }

    uint8_t * dst = my_buf; // (uint8_t*)malloc(10*1000*1000);
    const uint8_t * const old_dst = dst;

    // png header
    {
        // png magic bytes..
        uint8_t magicNumber[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
        memcpy(dst, magicNumber, sizeof(magicNumber));
        dst += 8;

        // maps #channels to png constant
        uint8_t channel_type[5] = {
            0, // Unused
            0, // GRAY
            4, // GRAY + ALPHA
            2, // RGB
            6  // RGBA
        };

        struct _png_header {
            uint32_t size;
            char name[4];
            uint32_t width;
            uint32_t height;
            uint8_t bpp;
            uint8_t channel_type;
            uint8_t compression_method;
            uint8_t filter_method;
            uint8_t interlace_mode;
            // wrong padding: uint32_t check_sum;
        };

        uint32_t check_sum;

        struct _png_header ihdr = {
            swap_endianness32(13),
            { 'I', 'H', 'D', 'R' },
            swap_endianness32(desc->width),
            swap_endianness32(desc->height),
            desc->bit_depth,
            channel_type[desc->num_channels],
            0, // filter is "sub" or "none"
            0, // filter
            0  // no interlacing supported currently
        };

        check_sum = swap_endianness32(crc32c(0xffffffff,
                                             (uint8_t*)ihdr.name, 4+13)^0xffffffff);
        memcpy(dst, &ihdr, 4+4+13);
        dst += 4+4+13;
        memcpy(dst, &check_sum, 4);
        dst += 4;
    }

    // png idat
    int64_t result = split(desc, dst);
    if(result < 0) {
        fclose(f);
        return (pngenc_result)result;
    }
    dst += result;

    // png end
    struct _png_end {
        uint32_t size;
        uint8_t name[4];
        uint32_t check_sum;
    } iend = {
        0, { 'I', 'E', 'N', 'D' }, 0
    };
    iend.check_sum = swap_endianness32(
                crc32c(0xffffffff, (uint8_t*)iend.name, 4) ^ 0xffffffff);
    memcpy(dst, &iend, sizeof(iend));
    fwrite(old_dst, (size_t)(dst - old_dst), 1, f);
    fclose(f);
    return PNGENC_SUCCESS;
}
