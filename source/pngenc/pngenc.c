#include "pngenc.h"
#include "splitter.h"
#include "image_descriptor.h"
#include "utils.h"
#include "crc32.h"
#include "adler32.h"
#include "callback.h"
#include <stdio.h>
#include <omp.h>

pngenc_encoder pngenc_create_encoder_default(void) {
    return pngenc_create_encoder(0, 512*1024);
}

pngenc_encoder pngenc_create_encoder(int32_t num_threads, uint32_t chunk_size) {
    init_cpu_info();
    crc32_init_sw();
    pngenc_encoder encoder =
            (pngenc_encoder)malloc(sizeof(struct _pngenc_encoder));
    encoder->num_threads = num_threads > 0
            ? num_threads
            : omp_get_max_threads();

    chunk_size = (chunk_size+15) & ~15; // align to 16 bytes
    // less than 64k per thread does not make much sense.
    chunk_size = max_u32(chunk_size, 1024*64);
    encoder->buffer_size = chunk_size;
    encoder->tmp_buffers = malloc(chunk_size * encoder->num_threads);

    // compressed data worst case: header + 15 bit per byte.
    // if output size is 2*input size we have one bit spare per input byte
    // as long as chunk_size is > 3K we're good
    // (max 300bytes header won't overflow with at least 300*8bytes input data)
    encoder->dst_buffers = malloc(2ULL * chunk_size * encoder->num_threads);
    return encoder;
}

int pngenc_write(pngenc_encoder encoder,
                 const pngenc_image_desc * descriptor,
                 const char * filename) {

    FILE * file = fopen(filename, "wb");
    if (!file)
        return PNGENC_ERROR_FILE_IO;

    int result = pngenc_encode(encoder, descriptor, write_to_file_callback, file);
    fclose(file);

    return result;
}

int pngenc_encode(pngenc_encoder encoder,
                  const pngenc_image_desc * descriptor,
                  pngenc_user_write_callback callback,
                  void *user_data) {

    return write_png(encoder, descriptor, callback, user_data);
}

void pngenc_destroy_encoder(pngenc_encoder encoder) {
    free(encoder->tmp_buffers);
    free(encoder->dst_buffers);
    free(encoder);
}

int pngenc_write_file(const pngenc_image_desc * descriptor,
                      const char * filename) {

    pngenc_encoder encoder = pngenc_create_encoder_default();
    RETURN_ON_ERROR(pngenc_write(encoder, descriptor, filename));
    pngenc_destroy_encoder(encoder);

    return PNGENC_SUCCESS;
}
