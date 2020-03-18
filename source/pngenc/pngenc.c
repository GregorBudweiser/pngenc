#include "pngenc.h"
#include "encoder.h"
#include "image_descriptor.h"
#include "utils.h"
#include "crc32.h"
#include "adler32.h"
#include "callback.h"
#include "png.h"
#include "decoder.h"
#include <stdio.h>
#include <omp.h>
#include <memory.h>

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
    int result = pngenc_write(encoder, descriptor, filename);
    pngenc_destroy_encoder(encoder);

    return result;
}

pngenc_decoder pngenc_create_decoder(void) {
    pngenc_decoder decoder =
            (pngenc_decoder)malloc(sizeof(struct _pngenc_decoder));
    const uint32_t N = 1024*1024;
    decoder->zlib.buf = malloc(N);
    memset(decoder->zlib.buf, 0, N);
    decoder->zlib.buf_size = N;
    decoder->zlib.state = UNINITIALIZED;
    decoder->zlib.deflate.bit_offset = 0;
    huffman_codec_init(&decoder->zlib.deflate.huff);
    return decoder;
}

void pngenc_destroy_decoder(pngenc_decoder decoder) {
    free(decoder);
}

int pngenc_read(pngenc_decoder decoder,
                pngenc_image_desc * descriptor,
                const char * filename) {

    FILE * file = fopen(filename, "rb");
    if (!file)
        return PNGENC_ERROR_FILE_IO;

    int result = pngenc_decode(decoder, descriptor, file);
    fclose(file);

    return result;
}

int pngenc_read_file(pngenc_image_desc * descriptor,
                     const char * filename) {

    pngenc_decoder decoder = pngenc_create_decoder();
    int result = pngenc_read(decoder, descriptor, filename);
    pngenc_destroy_decoder(decoder);
    return result;
}
