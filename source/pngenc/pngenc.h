#pragma once
#include <stdint.h>

#ifdef __MSVCRT__
    #undef PNGENC_EXPORT
    #if PNGENC_EXPORT
        #define PNGENC_EXPORT __declspec( dllexport )
    #else
        #define PNGENC_EXPORT __declspec( dllimport )
    #endif
#else
    #undef PNGENC_EXPORT
    #define PNGENC_EXPORT
#endif

#define PNGENC_VERSION_MAJOR 0
#define PNGENC_VERSION_MINOR 1

typedef enum _pngenc_compression_strategy {
    PNGENC_NO_COMPRESSION,                   // fastest, no compression
    PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1 // reasonably balanced
} pngenc_compression_strategy;

typedef enum _pngenc_result {
    PNGENC_SUCCESS           =  0,
    PNGENC_ERROR             = -1,
    PNGENC_ERROR_FILE_IO     = -2,
    PNGENC_ERROR_INVALID_ARG = -3,
    PNGENC_ERROR_FAILED_NODE_DESTROY = -4
} pngenc_result;

typedef struct _pngenc_image_desc {
    const uint8_t * data;
    uint64_t row_stride;
    uint32_t width;
    uint32_t height;
    pngenc_compression_strategy strategy;
    uint8_t num_channels;
} pngenc_image_desc;

typedef int (*pngenc_user_write_callback)(const void * data, uint32_t data_len,
                                          void * user_data);

/**
 * Simply write png to specified file.
 */
PNGENC_EXPORT int write_png_file(const pngenc_image_desc * descriptor,
                                 const char * filename);

/**
 * Encode png and use callback for storing to custom location (e.g. in memory).
 */
PNGENC_EXPORT int write_png_func(const pngenc_image_desc * descriptor,
                                 pngenc_user_write_callback write_data_callback,
                                 void * user_data);
