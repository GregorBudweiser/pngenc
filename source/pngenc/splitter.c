#include "splitter.h"
#include "adler32.h"
#include "huffman.h"
#include "crc32.h"
#include "utils.h"
#include "png.h"
#include "deflate.h"
#include "image_descriptor.h"
#include <string.h>
#include <stdio.h>
#include <omp.h>

#if defined(_MSC_VER) && defined(_WIN64)
#include <emmintrin.h>
#endif

uint16_t swap_uint16(uint16_t val) {
    return (uint16_t)(val << 8) | (uint16_t)(val >> 8);
}

/**
 * In case of compression
 */
uint32_t prepare_data_filtered(const pngenc_image_desc * image,
                               uint32_t yStart, uint32_t yEnd, uint8_t * dst) {
    uint64_t y;
    const uint64_t bytes_per_row = get_num_bytes_per_row(image);
    for(y = yStart; y < yEnd; y++) {
        const uint8_t * src = image->data + y*image->row_stride;
        *dst++ = 1; // row filter
        if(image->bit_depth == 8) {
            // delta-x row filter
            const uint8_t c = image->num_channels;

            uint64_t i;
            for(i = 0; i < c; i++)
                dst[i] = src[i];
#if defined(_MSC_VER) && defined(_WIN64)
            // Use SSE impl only for MSVC. GCC and Clang do this already
            for(i = 0; i < bytes_per_row-32; i+=16) {
                __m128i a = _mm_loadu_si128((const __m128i*)(src + i));
                __m128i b = _mm_loadu_si128((const __m128i*)(src +c + i));
                _mm_storeu_si128((__m128i*)(dst + c + i), _mm_sub_epi8(b, a));
            }
            // trailing bytes
            for(; i < bytes_per_row; i++)
                dst[i] = src[i] - src[i-c];
#else
            for(i = c; i < bytes_per_row; i++)
                dst[i] = src[i] - src[i-c];
#endif
            dst += bytes_per_row;

        } else { // 16bit
            // delta-x row filter filters MSB and LSB separately
            // i.e. the filter is byte based rather than word based
            const uint8_t c2 = image->num_channels*2;

            uint64_t i;
            for(i = 0; i < c2; i+=2) {
                dst[i] = src[i+1];
                dst[i+1] = src[i];
            }

            for(i = c2; i < bytes_per_row; i+=2) {
                dst[i] = src[i+1] - src[i+1-c2];
                dst[i+1] = src[1] - src[1-c2];
            }

            dst += bytes_per_row;
        }
    }

    return (yEnd - yStart)*(bytes_per_row + 1);
}

/**
 * Uncompressed path.
 */
uint32_t prepare_data_unfiltered(const pngenc_image_desc * image,
                                 uint32_t yStart, uint32_t yEnd,
                                 uint8_t * dst) {
    uint64_t y;
    const uint32_t bytes_per_row = get_num_bytes_per_row(image);
    for(y = yStart; y < yEnd; y++) {
        *dst++ = 0; // row filter
        const uint8_t * src = image->data + y*image->row_stride;
        memcpy(dst, src, bytes_per_row);
        dst += bytes_per_row;
    }

    return (yEnd - yStart)*(bytes_per_row + 1);
}

/**
 * Main loop (parallel) over image rows
 */
int split(const pngenc_encoder encoder, const pngenc_image_desc * desc,
          pngenc_user_write_callback callback, void * user_data) {

    pngenc_adler32 sum;
    adler_init(&sum);
    uint32_t num_rows = encoder->buffer_size/desc->row_stride + 1;
    int64_t y;

    uint32_t num_iterations = (desc->height - 1)/num_rows + 1;
    omp_set_num_threads(min_i32((int32_t)num_iterations,
                                omp_get_max_threads()));

    // zlib stream header..
    uint8_t shdr[2];
    shdr[0] = 0x08; // CM = 8 (=deflate), CINFO = 0 (window size)
    shdr[1] = (31 - (((uint32_t)shdr[0]*256) % 31))
            | (0 << 5) | (0 << 6); // FCHECK | FDICT | FLEVEL;
    RETURN_ON_ERROR(write_idat_block(shdr, 2, callback, user_data));

    int32_t err = 0;

#pragma omp parallel
#pragma omp for ordered schedule(static, 1)
    for(y = 0; y < desc->height; y += num_rows) {
        uint32_t yNext = min_u32((uint32_t)y+num_rows, desc->height);

        // intermediate result buffers
        uint8_t * tmp = encoder->tmp_buffers + (encoder->buffer_size*2*(uint32_t)omp_get_thread_num());
        uint8_t * dst = encoder->dst_buffers + (encoder->buffer_size*2*(uint32_t)omp_get_thread_num());

        pngenc_adler32 local_sum;
        adler_init(&local_sum);
        uint32_t num_bytes;
        int64_t result;
        uint8_t * const idat_ptr = dst;
        {
            uint32_t yStart = (uint32_t)y;
            uint32_t yEnd = yNext;

            // prepare data
            num_bytes = desc->strategy == PNGENC_NO_COMPRESSION
                    ? prepare_data_unfiltered(desc, yStart, yEnd, tmp)
                    : prepare_data_filtered(desc, yStart, yEnd, tmp);

            // update adler
            adler_update(&local_sum, tmp, num_bytes);

            // Idat hdr (part I)
            idat_header hdr = { 0, { 'I', 'D', 'A', 'T' } };
            dst += sizeof(hdr); // advance destination pointer

            // compress: zlib-deflate-block, zlib-uncompressed-0-block (zflush)
            uint8_t last_block = yEnd >= desc->height ? 1 : 0;
            result = desc->strategy == PNGENC_NO_COMPRESSION
                    ? write_deflate_block_uncompressed(dst, tmp, num_bytes, last_block)
                    : write_deflate_block_compressed(dst, tmp, num_bytes, last_block);

            if(result < 0) {
                err = 1;
                continue;
            }

            dst += result;

            // Idat hdr (part II)
            hdr.size = swap_endianness32((uint32_t)result);
            memcpy(idat_ptr, &hdr, 8);

            // Idat end/checksum (starts with crc("IDAT"))
            uint32_t crc = crc32c(0xCA50F9E1, idat_ptr+8, (size_t)result);
            crc = swap_endianness32(crc ^ 0xFFFFFFFF);
            memcpy(dst, &crc, 4);
            dst += 4;

            // number of bytes: Idat hdr + data + checksum
            result = (int64_t)(dst - idat_ptr);
        }

#pragma omp ordered
        {
            if(callback(idat_ptr, (uint32_t)result, user_data) < 0) {
                err = 1;
            }

            // update adler
            uint32_t comb = adler32_combine(adler_get_checksum(&sum),
                                            adler_get_checksum(&local_sum),
                                            num_bytes);
            adler_set_checksum(&sum, comb);
        }
    }

    if (err) {
        return PNGENC_ERROR;
    }

    // write adler checksum in its own idat block
    uint32_t adler_checksum = swap_endianness32(adler_get_checksum(&sum));
    RETURN_ON_ERROR(write_idat_block((uint8_t*)&adler_checksum, 4, callback, user_data));

    return PNGENC_SUCCESS;
}

pngenc_result write_png(const pngenc_encoder encoder,
                        const pngenc_image_desc * desc,
                        pngenc_user_write_callback callback,
                        void * user_data) {
    // sanity check of input
    RETURN_ON_ERROR(check_descriptor(desc));

    // actual work
    RETURN_ON_ERROR(write_png_header(desc, callback, user_data));
    RETURN_ON_ERROR(split(encoder, desc, callback, user_data));
    RETURN_ON_ERROR(write_png_end(callback, user_data));

    return PNGENC_SUCCESS;
}
