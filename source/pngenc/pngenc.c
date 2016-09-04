#include "pngenc.h"
#include "utils.h"
#include "crc32.h"
#include "adler32.h"
#include "node.h"
#include "node_custom.h"
#include "node_data_gen.h"
#include "node_deflate.h"
#include "node_idat.h"
#include <stdio.h>
#include <assert.h>

int write_ihdr(const pngenc_image_desc * descriptor,
               pngenc_user_write_callback callback, void * user_data);
int write_idat(const pngenc_image_desc * descriptor,
               pngenc_user_write_callback callback, void * user_data);
int write_iend(pngenc_user_write_callback callback, void * user_data);

int check_descriptor(const pngenc_image_desc * descriptor);

int write_to_file_callback(const void * data, uint32_t data_len,
                           void * user_data);
int write_image_data(const pngenc_image_desc * descriptor,
                     pngenc_user_write_callback callback, void * user_data);

int write_png_file(const pngenc_image_desc * descriptor,
                   const char * filename) {
    FILE * file;
    int result;

    if(filename == 0)
        return PNGENC_ERROR_INVALID_ARG;

    file = fopen(filename, "wb");
    if(file == NULL)
        return PNGENC_ERROR_FILE_IO;

    result = write_png_func(descriptor, &write_to_file_callback, file);
    if(fclose(file) != 0)
        return PNGENC_ERROR_FILE_IO;

    return result;
}

int write_png_func(const pngenc_image_desc * descriptor,
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
        8, // bit depth
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

int64_t user_write_wrapper(struct _pngenc_node * n, const uint8_t * data,
                           uint32_t size) {
    pngenc_node_custom * node = (pngenc_node_custom*)n;
    pngenc_user_write_callback user_provided_function = node->custom_data1;
    RETURN_ON_ERROR(user_provided_function(data, size, node->custom_data0));
    return size;
}

int write_image_data(const pngenc_image_desc * descriptor,
         pngenc_user_write_callback callback, void * user_data) {

    // Setup chain
    pngenc_node_data_gen node_data_gen;
    pngenc_node_deflate node_deflate;
    pngenc_node_idat node_idat;
    pngenc_node_custom node_user;
    node_data_generator_init(&node_data_gen, descriptor);
    node_deflate_init(&node_deflate, descriptor);
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

int write_row_uncompressed(const pngenc_image_desc * descriptor,
                           uint32_t row, pngenc_adler32 * zlib_check_sum,
                           int (*write_data_callback)(void * data,
                                                      uint32_t data_len,
                                                      void * user_data),
                           void * user_data) {
    struct _pngenc_data {
        uint32_t size;
        char name[4];
    };

    struct _pngenc_deflate_header {
        uint8_t compression_method;
        uint8_t compression_flags;
    };

    struct _pngenc_deflate_block {
        uint8_t padding;
        uint8_t deflate_block_header;
        uint16_t raw_data_size;
        uint16_t raw_data_size_inverted;
    };

    struct _pngenc_data idat =
    {
        0, // idat header + zlib crc
        { 'I', 'D', 'A', 'T' }
    };

    struct _pngenc_deflate_header zlib_hdr;
    // compression method (8 = deflate; png only supports this) and log2 base
    // of search window (7 means 32k window)
    zlib_hdr.compression_method = (1 << 4) | 8;
    zlib_hdr.compression_flags = (31 - (zlib_hdr.compression_method*256 % 31))
                               | (0 << 5) | (0 << 6); // FCHECK | FDICT | FLEVEL

    const int is_first_block = (int)(row == 0);
    const int is_final_block = (int)(row == descriptor->height-1);
    const uint32_t row_size = descriptor->width * descriptor->num_channels;

    // idat content: compression method and flags
    idat.size = swap_endianness32((is_first_block ? 2 : 0)
                                + 5   // deflate_block  header
                                + 1   // filter type
                                + row_size // raw data
                                + (is_final_block ? 4 : 0));

    // Add idat header
    RETURN_ON_ERROR(write_data_callback(&idat, 4+4, user_data));

    // Init checksums
    uint8_t row_filter = 0; // rowfilter: 0 = no rowfilter
    uint32_t idat_check_sum = crc32c(0xffffffff, (uint8_t*)&idat.name, 4);

    if(is_first_block) {
        idat_check_sum = crc32c(idat_check_sum, (uint8_t*)&zlib_hdr, 2);
        RETURN_ON_ERROR(write_data_callback((uint8_t*)&zlib_hdr, 2, user_data));
    }

    // One block per scanline.. TODO: split if needed..
    int raw_data_size = row_size + 1; // +1 byte for rowfilter
    assert(raw_data_size <= 0xFFFF); // 16 bit size (e.g. max 64k per block)

    // zlib stuff
    struct _pngenc_deflate_block block =
    {
        0, // Unused (padding)
        is_final_block, // Final block or not?
        (uint16_t)raw_data_size,
        (uint16_t)~raw_data_size
    };

    // Add block header
    idat_check_sum = crc32c(idat_check_sum,
                            (uint8_t*)&block.deflate_block_header, 5);
    RETURN_ON_ERROR(write_data_callback((uint8_t*)&block.deflate_block_header,
                                        5, user_data));

    // Write data while computing it's crc
    // Data
    RETURN_ON_ERROR(write_data_callback((uint8_t*)&row_filter, 1, user_data));
    RETURN_ON_ERROR(write_data_callback(((uint8_t*)descriptor->data)
                                        + row*descriptor->row_stride,
                                        row_size, user_data));

    // AdlerCRC
    adler_update(zlib_check_sum, (uint8_t*)&row_filter, 1);
    adler_update(zlib_check_sum, ((uint8_t*)descriptor->data)
                 + row*descriptor->row_stride, row_size);

    // CRC32
    idat_check_sum = crc32c(idat_check_sum, (uint8_t*)&row_filter, 1);
    idat_check_sum = crc32c(idat_check_sum,
                            ((uint8_t*)descriptor->data)
                            + row*descriptor->row_stride, row_size);

    // Add adler crc to end of zlib data-stream
    if(is_final_block)
    {
        uint32_t adler_check_sum = swap_endianness32(
                    adler_get_checksum(zlib_check_sum));
        RETURN_ON_ERROR(write_data_callback((uint8_t*)&adler_check_sum, 4,
                                            user_data));

        printf("adler: 0x%08x\n", swap_endianness32(adler_check_sum));

        // Adler checksum is part of deflate block and included in IDAT CRC!
        idat_check_sum = crc32c(idat_check_sum, (uint8_t*)&adler_check_sum, 4);
    }

    idat_check_sum = swap_endianness32(idat_check_sum ^ 0xffffffff);
    RETURN_ON_ERROR(write_data_callback((uint8_t*)&idat_check_sum, 4,
                                        user_data));

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

int check_descriptor(const pngenc_image_desc * descriptor) {
    if(descriptor->data == NULL)
        return PNGENC_ERROR_INVALID_ARG;

    if(descriptor->width == 0 || descriptor->height == 0)
        return PNGENC_ERROR_INVALID_ARG;

    if(descriptor->num_channels > 4)
        return PNGENC_ERROR_INVALID_ARG;

    if((uint64_t)descriptor->row_stride < (uint64_t)descriptor->width
                                        * (uint64_t)descriptor->num_channels)
        return PNGENC_ERROR_INVALID_ARG;

    return PNGENC_SUCCESS;
}

int write_to_file_callback(const void * data, uint32_t data_len,
                           void * user_data) {
    uint32_t bytesWritten;

    bytesWritten = (uint32_t)fwrite(data, 1, data_len, (FILE*)user_data);
    return bytesWritten == data_len
            ? PNGENC_SUCCESS
            : PNGENC_ERROR_FILE_IO;
}
