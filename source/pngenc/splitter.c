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

uint16_t swap_uint16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

/**
 * In case of compression
 */
uint32_t prepare_data_filtered(const pngenc_image_desc * image,
                               uint32_t yStart, uint32_t yEnd, uint8_t * dst) {
    uint64_t y;
    const uint64_t length = image->width * image->num_channels
                          * (image->bit_depth / 8);
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

uint32_t prepare_data_unfiltered(const pngenc_image_desc * image,
                                 uint32_t yStart, uint32_t yEnd,
                                 uint8_t * dst) {
    uint64_t y;
    const uint32_t length = image->width * image->num_channels
                          * (image->bit_depth / 8);
    for(y = yStart; y < yEnd; y++) {
        *dst++ = 0; // row filter
        const uint8_t * src = image->data + y*image->row_stride;
        memcpy(dst, src, length);
        dst +=length;
    }

    return (yEnd - yStart)*(length + 1);
}


int64_t split(const pngenc_encoder encoder, const pngenc_image_desc * desc,
              pngenc_user_write_callback callback, void * user_data) {

    pngenc_adler32 sum;
    adler_init(&sum);
    uint32_t num_rows = encoder->buffer_size/desc->row_stride + 1;
    uint32_t y;

    int32_t err = 0;

#pragma omp parallel
#pragma omp for ordered
    for(y = 0; y < desc->height; y += num_rows) {
        uint32_t yNext = min_u32(y+num_rows, desc->height);

        // intermediate result buffers
        uint8_t * tmp = encoder->tmp_buffers + (encoder->buffer_size*2*(uint32_t)omp_get_thread_num());
        //memset(tmp, 0, 2*encoder->buffer_size);
        uint8_t * dst = encoder->dst_buffers + (encoder->buffer_size*2*(uint32_t)omp_get_thread_num());
        //memset(dst, 0, 2*encoder->buffer_size);

        pngenc_adler32 local_sum;
        adler_init(&local_sum);
        uint32_t num_bytes;
        int64_t result;
        uint8_t * const idat_ptr = dst;
        {
            uint32_t yStart = y;
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
            result = desc->strategy == PNGENC_NO_COMPRESSION
                    ? write_deflate_block_uncompressed(dst, tmp, num_bytes)
                    : write_deflate_block_compressed(dst, tmp, num_bytes, 0);

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
            callback(idat_ptr, (uint32_t)result, user_data);

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
    write_idat_block((uint8_t*)&adler_checksum, 4, callback, user_data);

    return PNGENC_SUCCESS;
}

pngenc_result write_png(const pngenc_encoder encoder,
                        const pngenc_image_desc * desc,
                        pngenc_user_write_callback callback,
                        void * user_data) {
    RETURN_ON_ERROR(check_descriptor(desc));

    // png header
    write_png_header(desc, callback, user_data);

    // png idat
    int64_t result = split(encoder, desc, callback, user_data);
    RETURN_ON_ERROR(result);

    // png end
    write_png_end(callback, user_data);
    return PNGENC_SUCCESS;
}
