#include "pngenc.h"
#include "splitter.h"
#include "image_descriptor.h"
#include "utils.h"
#include "crc32.h"
#include "adler32.h"
#include "callback.h"
#include <stdio.h>
#include <omp.h>

pngenc_encoder pngenc_create_encoder(void) {
    pngenc_encoder encoder =
            (pngenc_encoder)malloc(sizeof(struct _pngenc_encoder));
    encoder->num_threads = omp_get_max_threads();
    encoder->buffer_size = 1024*1024; // src=1MB; dst=2MB
    encoder->tmp_buffers = malloc(2ULL * encoder->buffer_size
                                  * (uint32_t)encoder->num_threads);
    encoder->dst_buffers = malloc(2ULL * encoder->buffer_size
                                  * (uint32_t)encoder->num_threads);
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

    pngenc_encoder encoder = pngenc_create_encoder();
    RETURN_ON_ERROR(pngenc_write(encoder, descriptor, filename));
    pngenc_destroy_encoder(encoder);

    return PNGENC_SUCCESS;
}
