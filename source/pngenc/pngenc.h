#pragma once
#include <stdint.h>

#ifdef _MSC_VER
    #if PNGENC_EXPORT
        #define PNGENC_API __declspec( dllexport )
    #else
        #define PNGENC_API __declspec( dllimport )
    #endif
#else
    #define PNGENC_API
#endif

#define PNGENC_VERSION_MAJOR 0
#define PNGENC_VERSION_MINOR 5
#define PNGENC_VERSION_PATCH 1

typedef enum _pngenc_compression_strategy {
    PNGENC_NO_COMPRESSION,                    // fastest, no compression
    PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1  // reasonably fast
} pngenc_compression_strategy;

typedef enum _pngenc_result {
    PNGENC_SUCCESS           =  0,
    PNGENC_ERROR             = -1,
    PNGENC_ERROR_FILE_IO     = -2,
    PNGENC_ERROR_INVALID_ARG = -3,
    PNGENC_ERROR_FAILED_NODE_DESTROY = -4,
    PNGENC_ERROR_NOT_A_PNG   = -5,
    PNGENC_ERROR_UNSUPPORTED = -6,
    PNGENC_ERROR_INVALID_CHECKSUM = -7
} pngenc_result;

typedef struct _pngenc_image_desc {
    uint8_t * data;
    uint32_t row_stride;
    uint32_t width;
    uint32_t height;
    pngenc_compression_strategy strategy;
    uint8_t num_channels;
    uint8_t bit_depth;
} pngenc_image_desc;

typedef int (*pngenc_user_write_callback)(const void * data, uint32_t data_len,
                                          void * user_data);

typedef struct _pngenc_encoder* pngenc_encoder;

typedef struct _pngenc_decoder* pngenc_decoder;

/**
 * Simply write png to specified file.
 */
PNGENC_API
int pngenc_write_file(const pngenc_image_desc * descriptor,
                      const char * filename);
PNGENC_API
int pngenc_read_file(pngenc_image_desc * descriptor,
                     const char * filename);

/**
 * Encode png and use callback for storing to custom location (e.g. in memory).
 */
PNGENC_API
int pngenc_write_func(const pngenc_image_desc * descriptor,
                      pngenc_user_write_callback write_data_callback,
                      void * user_data);

PNGENC_API
pngenc_encoder pngenc_create_encoder(void);

PNGENC_API
int pngenc_write(pngenc_encoder encoder,
                 const pngenc_image_desc * descriptor,
                 const char * file);

PNGENC_API
int pngenc_encode(pngenc_encoder encoder,
                  const pngenc_image_desc * descriptor,
                  pngenc_user_write_callback callback,
                  void *user_data);

PNGENC_API
void pngenc_destroy_encoder(pngenc_encoder encoder);
