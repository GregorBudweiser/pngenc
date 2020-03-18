#pragma once

#include "pngenc.h"

struct _pngenc_encoder {
    int32_t num_threads;
    uint32_t buffer_size; // multiple of 64
    uint8_t * tmp_buffers; // ptr to first buffer; must align to 64 bytes
    uint8_t * dst_buffers; // ptr to first buffer; must align to 64 bytes
};
typedef struct _pngenc_encoder* pngenc_encoder;

int split(const pngenc_encoder encoder, const pngenc_image_desc * desc,
          pngenc_user_write_callback callback, void * user_data);

pngenc_result write_png(const pngenc_encoder encoder,
                        const pngenc_image_desc * desc,
                        pngenc_user_write_callback callback, void *user_data);
