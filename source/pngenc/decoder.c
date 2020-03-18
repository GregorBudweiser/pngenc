#include "decoder.h"
#include "utils.h"
#include "buffer.h"
#include "assert.h"
#include "deflate.h"
#include "png.h"
#include "memory.h"
#include "malloc.h"
#include "image_descriptor.h"
#include "crc32.h"

int pngenc_decode(pngenc_decoder decoder,
                  pngenc_image_desc * descriptor,
                  FILE * file) {
    memset(descriptor, 0, sizeof(pngenc_image_desc));
    RETURN_ON_ERROR(read_png_header(descriptor, file))
    uint64_t num_bytes_img_data = get_num_bytes_per_row(descriptor)
                                * descriptor->height;

    descriptor->row_stride = (uint32_t)get_num_bytes_per_row(descriptor);

    if(num_bytes_img_data > 0xFFFFFFFF || descriptor->bit_depth != 8) {
        return PNGENC_ERROR_UNSUPPORTED;
    }
    descriptor->data = (uint8_t*)malloc((size_t)num_bytes_img_data);
    memset(descriptor->data, 0, (size_t)num_bytes_img_data);
    if(descriptor->data == 0) {
        return PNGENC_ERROR_INVALID_ARG; // TODO: out of mem?
    }

    uint32_t offset = 0;
    png_chunk chunk;
    memset(&chunk, 0, sizeof(png_chunk));
    while(memcmp(chunk.name, "IEND", 4) != 0) {
        RETURN_ON_ERROR(read_png_chunk_header(&chunk, file))
        //printf("got chunk: %c%c%c%c\n", chunk.name[0],
        //       chunk.name[1], chunk.name[2], chunk.name[3]);

        if(memcmp(chunk.name, "IDAT", 4) == 0) {
            uint32_t bytes_remain = chunk.size;
            while(bytes_remain) {
                uint32_t space_avail = decoder->zlib.buf_size - offset;
                uint32_t bytes_to_read = min_u32(space_avail, bytes_remain);
                fread(decoder->zlib.buf + offset, 1, bytes_to_read, file);
                bytes_remain -= bytes_to_read;
                offset += bytes_to_read;
                if(offset == decoder->zlib.buf_size) {
                    int64_t bytes_decoded = decode_zlib_stream(
                                descriptor->data, (size_t)num_bytes_img_data,
                                decoder->zlib.buf, decoder->zlib.buf_size, decoder);
                }
            }
            //printf("added %ukb..\n", chunk.size/1024);

            uint32_t checksum_computed = 0xffffffff;
            checksum_computed = crc32c(checksum_computed, chunk.name, 4);
            checksum_computed = crc32c(checksum_computed, decoder->zlib.buf+offset-chunk.size, chunk.size);
            checksum_computed = swap_endianness32(checksum_computed ^ 0xffffffff);

            uint32_t checksum_read = 0;
            fread(&checksum_read, sizeof(uint32_t), 1, file);
            if(checksum_computed != checksum_read) {
                return PNGENC_ERROR_INVALID_CHECKSUM;
            }

        } else {
            // TODO: handle sizes > 2GB
            assert(chunk.size <= 0x7FFFFFFF);
            int seek = fseek(file, 4+(int)chunk.size, SEEK_CUR);
        }
    }
    if(offset) {
        // TODO..
        uint8_t zlib_output_buf[1024*1024];


        int64_t bytes_decoded = decode_zlib_stream(
                    zlib_output_buf, (int32_t)num_bytes_img_data,
                    decoder->zlib.buf, offset, &decoder->zlib);

        printf("Decoded %d bytes..\n", (int)bytes_decoded);

        for(uint32_t y = 0; y < descriptor->height; y++) {
            const uint8_t * src = zlib_output_buf + y*(get_num_bytes_per_row(descriptor)+1);
            uint8_t row_filter = *src++;
            uint8_t * dst = descriptor->data + y*get_num_bytes_per_row(descriptor);
            uint8_t * prior = descriptor->data + (y-1)*get_num_bytes_per_row(descriptor);
            // TODO: only works for uint8_t
            switch(row_filter) {
                case 0: {
                    for(uint32_t x = 0; x < get_num_bytes_per_row(descriptor); x++) {
                        dst[x] = src[x];
                    }
                    break;
                }
                case 1: {
                    for(uint32_t x = 0; x < descriptor->num_channels; x++) {
                        dst[x] = src[x];
                    }
                    for(uint32_t x = descriptor->num_channels; x < get_num_bytes_per_row(descriptor); x++) {
                        dst[x] = dst[x-descriptor->num_channels] + src[x];
                    }
                    break;
                }
                case 2: {
                    for(uint32_t x = 0; x < get_num_bytes_per_row(descriptor); x++) {
                        dst[x] = src[x] + prior[x];
                    }
                    break;
                }
                case 3: {
                    for(uint32_t x = 0; x < get_num_bytes_per_row(descriptor); x++) {
                        dst[x] = src[x] - prior[x];
                    }
                    break;
                }
                case 4: {
                    for(uint32_t x = 0; x < descriptor->num_channels; x++) {
                        dst[x] = src[x] + paeth_predictor(0, prior[x], 0);
                    }
                    for(uint32_t x = descriptor->num_channels; x < get_num_bytes_per_row(descriptor); x++) {
                        // dst = paeth(x) + prediction of surrounding pixels
                        dst[x] = src[x] + paeth_predictor(dst[x-descriptor->num_channels], prior[x],
                                                          prior[x-descriptor->num_channels]);
                    }
                    break;
                }

                default: {
                    printf("Row %d: invalid row filter: %d\n", (int)y, (int)row_filter);
                    for(uint32_t x = 0; x < get_num_bytes_per_row(descriptor); x++) {
                        dst[x] = 0;
                    }
                    break;
                }
            }
        }
    }

    return PNGENC_SUCCESS;
}

uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c) {
        // a = left, b = above, c = upper left
        int32_t p = (uint32_t)a + (uint32_t)b - (uint32_t)c;       // initial estimate
        int pa = abs(p - a);      // distances to a, b, c
        int pb = abs(p - b);
        int pc = abs(p - c);
        // return nearest of a,b,c,
        // breaking ties in order a,b,c.
        if (pa <= pb && pa <= pc)
            return a;
        else if (pb <= pc)
             return b;
        else
             return c;
}

// pretty picture
uint8_t paeth_predictor2(uint8_t a, uint8_t b, uint8_t c) {
        // a = left, b = above, c = upper left
        uint8_t p = (uint8_t)(a + b) - c;       // initial estimate
        int pa = abs((uint8_t)(p - a));      // distances to a, b, c
        int pb = abs((uint8_t)(p - b));
        int pc = abs((uint8_t)(p - c));
        // return nearest of a,b,c,
        // breaking ties in order a,b,c.
        if (pa <= pb && pa <= pc)
            return a;
        else if (pb <= pc)
             return b;
        else
             return c;
}
