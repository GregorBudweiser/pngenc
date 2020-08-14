#include "zlib.h"
#include "pngenc.h"
#include "utils.h"
#include <stdio.h>

int64_t decode_zlib_stream(uint8_t * dst, int32_t dst_size,
                           const uint8_t * src, uint32_t src_size,
                           zlib_codec *decoder) {

    printf("decode %ukb..\n", src_size/1024);
    // TODO: check dst_size > 300 bytes...

    switch(decoder->state) {
        case UNINITIALIZED:
        {
            // TODO: read zlib stream header
            uint8_t compression_method = src[0] & 0xF;
            uint8_t window_size = src[0] >> 4;
            printf("CM: %d, WS: %d\n", (int)compression_method, (int)window_size);
            uint8_t fcheck = src[1] & 0x1F;
            uint8_t fdict = (src[1] >> 5) & 0x1;
            uint8_t flevel = (src[1] >> 6);
            printf("fcheck: %d, fdict: %d, flevel %d\n", (int)fcheck, (int)fdict, (int)flevel);
            if((src[0]*256 + src[1]) % 31) {
                return PNGENC_ERROR_INVALID_CHECKSUM;
            }
            decoder->state = READY;
            decoder->deflate.bit_offset = 16; // we just consumed first two bytes

            printf("%d %d | %d %d\n", (int)src[0], (int)src[1], (int)src[2], (int)src[3]);
            fflush(stdout);
        }
            // fallthrough
        case READY:
        {
            uint32_t is_last_block;
            do {
                is_last_block = pop_bits(1, src, &decoder->deflate.bit_offset);
                uint32_t block_type = pop_bits(2, src, &decoder->deflate.bit_offset);
                printf("last: %d, type: %d\n", (int)is_last_block, (int)block_type);
                switch(block_type) {
                    case 2: // dynamic compressed header
                    {
                        RETURN_ON_ERROR(read_dynamic_header(src, src_size, &decoder->deflate))
                        int ret = decode_data_full(&decoder->deflate, src, src_size, dst, dst_size);
                        if(ret < 0) {
                            return PNGENC_ERROR;
                        }
                        dst += ret;
                        dst_size -= ret;
                        break;
                    }
                    case 1: // static compressed
                        RETURN_ON_ERROR(read_static_header(src, src_size, &decoder->deflate))
                        int ret = decode_data_full(&decoder->deflate, src, src_size, dst, dst_size);
                        if(ret < 0) {
                            return PNGENC_ERROR;
                        }
                        dst += ret;
                        dst_size -= ret;
                        break;
                    case 0: // uncompressed
                    default:
                        printf("Compression type %d not supported!\n", block_type);
                        return PNGENC_ERROR_UNSUPPORTED;
                }
            } while(!is_last_block);
        }

        case NEEDS_MORE_INPUT:
            break;
        case DONE:
            return 0; // TODO: this should not happen
    }

    return 0;
}
