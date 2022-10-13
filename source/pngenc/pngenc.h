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
#define PNGENC_VERSION_MINOR 7
#define PNGENC_VERSION_PATCH 0

/**
 * Compression strategy; influences both png and zlib/deflate settings.
 */
typedef enum _pngenc_compression_strategy {
    /**
     * No compression, no filtering. Fastest option.
     */
    PNGENC_NO_COMPRESSION,

    /**
     * Reasonably fast option with reasonably good compression.
     * Uses huffman coding (Z_HUFF_ONLY) and horizontal row-filter.
     */
    PNGENC_HUFF_ONLY,

    /**
     * Good compression.
     * Uses run length encoding and horizontal row-filter.
     */
    PNGENC_RLE
} pngenc_compression_strategy;

/**
 * Result type to indicate success or specific error.
 */
typedef enum _pngenc_result {
    /**
     * The operation succeeded.
     */
    PNGENC_SUCCESS           =  0,

    /**
     * Generic error.
     */
    PNGENC_ERROR             = -1,

    /**
     * The error while writing to file.
     */
    PNGENC_ERROR_FILE_IO     = -2,

    /**
     * One or more of the arguments are invalid.
     */
    PNGENC_ERROR_INVALID_ARG = -3,

    /**
     * The chunk_size is too small for the requested image.
     * This happens only if a single image row does not fit into
     * chunk_size (which is 1MB by default). @see pngenc_create_encoder().
     */
    PNGENC_ERROR_INVALID_BUF = -4
} pngenc_result;

/**
 * Image descriptor
 */
typedef struct _pngenc_image_desc {
    /**
     * Pointer to image data.
     */
    const uint8_t * data;

    /**
     * Number of bytes per row + possibly padding.
     * Must at least be width*height*num_channels*bit_depth/8.
     */
    uint32_t row_stride;

    /**
     * Image width in pixels.
     */
    uint32_t width;

    /**
     * Image height in pixels.
     */
    uint32_t height;

    /**
     * Compression strategy to use.
     */
    pngenc_compression_strategy strategy;

    /**
     * Number of image channels.
     */
    uint8_t num_channels;

    /**
     * Number of bits per pixel and channel. Must be multiple of 8.
     */
    uint8_t bit_depth;
} pngenc_image_desc;

/**
 * Callback signature for custom output.
 *
 * @param data chunk of encoded data
 * @param data_len number of bytes
 * @param pointer to user data provided in @see pngenc_encode()
 * @returns non-zero in case of error
 */
typedef int (*pngenc_user_write_callback)(const void * data, uint32_t data_len,
                                          void * user_data);
/**
 * PNG encoder.
 */
struct _pngenc_encoder {
    /**
     * Number of threads used.
     */
    int32_t num_threads;

    /**
     * Internal buffer size.
     */
    uint32_t buffer_size;

    /**
     * Buffer containing intermediate data (i.e. filtered data).
     *
     * This buffer is partitioned and used by all threads.
     */
    uint8_t * tmp_buffers;

    /**
     * Buffer containing encoded data.
     *
     * This buffer is partitioned and used by all threads.
     */
    uint8_t * dst_buffers;
};
typedef struct _pngenc_encoder* pngenc_encoder;

/**
 * Write an image to file.
 *
 * @param descriptor: Image descriptor.
 *
 * @param filename: Name of the output file.
 *
 * @returns: Integer representing a pngenc_result.
 */
PNGENC_API
int pngenc_write_file(const pngenc_image_desc * descriptor,
                      const char * filename);

/**
 * Create default encoder.
 *
 * @returns: Encoder handle.
 */
PNGENC_API
pngenc_encoder pngenc_create_encoder_default();

/**
 * Create encoder.
 *
 * @param num_threads: The number of threads to be used. A value of 0 or less
 *        will use a number of threads equal to the number of logical
 *        processors.
 *
 * @param chunk_size: Each thread will process chunk_size bytes of the input
 *        at once. Higher numbers cause better compression but possibly less
 *        parallelization and requires more memory. The default is ~1MB.
 *        For best performance, the chunk size must be a multiple of the
 *        processor's cache line size (typically 64 bytes).
 *
 * @returns: Encoder handle.
 */
PNGENC_API
pngenc_encoder pngenc_create_encoder(int32_t num_threads, uint32_t chunk_size);

/**
 * Write image to file.
 *
 * @param encoder: Encoder handle.
 *
 * @param descriptor: Image descriptor.
 *
 * @param filename: Name of output file.
 *
 * @returns: Integer representing a pngenc_result.
 */
PNGENC_API
int pngenc_write(pngenc_encoder encoder,
                 const pngenc_image_desc * descriptor,
                 const char * filename);

/**
 * Encode image and use callback receiving processed data.
 *
 * @param encoder: Encoder handle.
 *
 * @param descriptor: Image descriptor.
 *
 * @param write_data_callback: Called whenever more chunks of data are
 *        available.
 *
 * @param user_data: Pointer to custom data that is provided in callback.
 *
 * @returns: Integer representing a pngenc_result.
 */
PNGENC_API
int pngenc_encode(pngenc_encoder encoder,
                  const pngenc_image_desc * descriptor,
                  pngenc_user_write_callback callback,
                  void *user_data);

/**
 * Destroy encoder.
 *
 * @param encoder: Encoder hanlde.
 */
PNGENC_API
void pngenc_destroy_encoder(pngenc_encoder encoder);
