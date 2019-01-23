#include "pngenc.h"
#include "image_descriptor.h"
#include "utils.h"
#include "crc32.h"
#include "adler32.h"
#include "node.h"
#include "node_custom.h"
#include "node_data_gen.h"
#include "node_deflate.h"
#include "node_idat.h"
#include "callback.h"
#include "png_pipeline.h"
#include <stdio.h>
#include <assert.h>

int write_ihdr(const pngenc_image_desc * descriptor,
               pngenc_user_write_callback callback, void * user_data);
int write_idat(const pngenc_image_desc * descriptor,
               pngenc_user_write_callback callback, void * user_data);
int write_iend(pngenc_user_write_callback callback, void * user_data);

int write_image_data(const pngenc_image_desc * descriptor,
                     pngenc_user_write_callback callback, void * user_data);
PNGENC_API
int pngenc_write_file(const pngenc_image_desc * descriptor,
                      const char * filename) {
    FILE * file;
    int result;

    if(filename == 0)
        return PNGENC_ERROR_INVALID_ARG;

    file = fopen(filename, "wb");
    if(file == NULL)
        return PNGENC_ERROR_FILE_IO;

    result = pngenc_write_func(descriptor, &write_to_file_callback, file);
    if(fclose(file) != 0)
        return PNGENC_ERROR_FILE_IO;

    return result;
}

PNGENC_API
int pngenc_write_func(const pngenc_image_desc * descriptor,
                      pngenc_user_write_callback write_data_callback,
                      void * user_data) {
    RETURN_ON_ERROR(check_descriptor(descriptor));
    RETURN_ON_ERROR(write_ihdr(descriptor, write_data_callback, user_data));
    RETURN_ON_ERROR(write_image_data(descriptor, write_data_callback,
                                     user_data));
    RETURN_ON_ERROR(write_iend(write_data_callback, user_data));
    return PNGENC_SUCCESS;
}

int write_ihdr(const pngenc_image_desc * descriptor,
               pngenc_user_write_callback write_data_callback,
               void * user_data) {
    // png magic bytes..
    uint8_t magicNumber[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    RETURN_ON_ERROR(write_data_callback(magicNumber, 8, user_data));

    // maps #channels to png constant
    uint8_t channel_type[5] = {
        0, // Unused
        0, // GRAY
        4, // GRAY + ALPHA
        2, // RGB
        6  // RGBA
    };

    struct _png_header {
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
    };

    uint32_t check_sum;

    struct _png_header ihdr = {
        swap_endianness32(13),
        { 'I', 'H', 'D', 'R' },
        swap_endianness32(descriptor->width),
        swap_endianness32(descriptor->height),
        descriptor->bit_depth,
        channel_type[descriptor->num_channels],
        0, // filter is "sub" or "none"
        0, // filter
        0  // no interlacing supported currently
    };

    check_sum = swap_endianness32(crc32c(0xffffffff,
                                         (uint8_t*)ihdr.name, 4+13)^0xffffffff);

    RETURN_ON_ERROR(write_data_callback((void*)&ihdr, 4+4+13, user_data));
    RETURN_ON_ERROR(write_data_callback((void*)&check_sum, 4, user_data));

    return PNGENC_SUCCESS;
}

int write_image_data(const pngenc_image_desc * descriptor,
                     pngenc_user_write_callback callback,
                     void * user_data) {

    // Setup chain
    pngenc_node_data_gen node_data_gen;
    pngenc_node_deflate node_deflate;
    pngenc_node_idat node_idat;
    pngenc_node_custom node_user;
    node_data_generator_init(&node_data_gen, descriptor);
    node_deflate_init(&node_deflate, descriptor->strategy);
    node_idat_init(&node_idat);

    node_user.base.next = 0;
    node_user.base.init = 0;
    node_user.base.write = &user_write_wrapper;
    node_user.base.finish = 0;
    node_user.custom_data0 = user_data;
    node_user.custom_data1 = callback;

    node_data_gen.base.next = (pngenc_node*)&node_deflate;
    node_deflate.base.next = (pngenc_node*)&node_idat;
    node_idat.base.next = (pngenc_node*)&node_user;

    RETURN_ON_ERROR(node_init((pngenc_node*)&node_data_gen));
    RETURN_ON_ERROR(node_write((pngenc_node*)&node_data_gen, 0, 1));
    RETURN_ON_ERROR(node_finish((pngenc_node*)&node_data_gen));

    node_destroy_data_generator(&node_data_gen);
    node_destroy_deflate(&node_deflate);
    node_destroy_idat(&node_idat);

    return PNGENC_SUCCESS;
}

PNGENC_API
pngenc_pipeline pngenc_pipeline_create(const pngenc_image_desc * descriptor,
                                       pngenc_user_write_callback callback,
                                       void * user_data) {
    uint32_t num_bytes = sizeof(struct _pngenc_pipeline);
    pngenc_pipeline pipeline = (pngenc_pipeline)malloc(num_bytes);
    png_encoder_pipeline_init(pipeline, descriptor,callback, user_data);
    return pipeline;
}

PNGENC_API
int pngenc_pipeline_write(pngenc_pipeline pipeline,
                          const pngenc_image_desc * descriptor,
                          void * user_data) {
    pipeline->node_user.custom_data0 = user_data;

    RETURN_ON_ERROR(check_descriptor(descriptor));
    RETURN_ON_ERROR(write_ihdr(descriptor, pipeline->node_user.custom_data1, user_data));
    RETURN_ON_ERROR(node_init((pngenc_node*)&pipeline->node_data_gen));
    RETURN_ON_ERROR(png_encoder_pipeline_write(pipeline, descriptor));
    RETURN_ON_ERROR(write_iend(pipeline->node_user.custom_data1, user_data));
    return PNGENC_SUCCESS;
}

PNGENC_API
int pngenc_pipeline_destroy(pngenc_pipeline pipeline) {
    RETURN_ON_ERROR(png_encoder_pipeline_destroy(pipeline));
    free(pipeline);
    return PNGENC_SUCCESS;
}

int write_iend(pngenc_user_write_callback callback, void * user_data) {
    struct _png_end {
        int size;
        char name[4];
        uint32_t check_sum;
    };

    struct _png_end iend = {
        0, { 'I', 'E', 'N', 'D' }, 0
    };
    iend.check_sum = swap_endianness32(
                crc32c(0xffffffff, (uint8_t*)iend.name, 4) ^ 0xffffffff);

    RETURN_ON_ERROR(callback((void*)&iend, 8+4, user_data));

    return PNGENC_SUCCESS;
}
