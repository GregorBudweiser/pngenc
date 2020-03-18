#pragma once
#include "inflate.h"
#include "deflate.h"

typedef enum _decoder_state {
    UNINITIALIZED,    // Not yet a zlib stream
    READY,            // Ready to read a new block
    NEEDS_MORE_INPUT, // Need more data to finish current block
    DONE              // Reached eind of zlib stream
} decoder_state;

typedef struct _zlib_codec {
    deflate_codec deflate;
    uint8_t * buf;
    decoder_state state;
    uint32_t buf_size;
} zlib_codec;

int64_t decode_zlib_stream(uint8_t * dst, int32_t dst_size,
                           const uint8_t * src, uint32_t src_size,
                           zlib_codec * decoder);
