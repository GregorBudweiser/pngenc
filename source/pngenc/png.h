#pragma once
#include <stdint.h>
#include "pngenc.h"

typedef struct _png_header {
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
} png_header;

typedef struct _idat_header {
    uint32_t size;
    uint8_t name[4];
} idat_header;

typedef struct _png_end {
    uint32_t size;
    uint8_t name[4];
    uint32_t check_sum;
} png_end;

int write_png_header(const pngenc_image_desc * desc,
                     pngenc_user_write_callback callback,
                     void * user_data);

int write_idat_block(const uint8_t * src, uint32_t len,
                     pngenc_user_write_callback callback,
                     void * user_data);

int write_png_end(pngenc_user_write_callback callback,
                  void * user_data);
