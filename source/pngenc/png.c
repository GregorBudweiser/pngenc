#include "png.h"
#include <string.h>
#include "crc32.h"
#include "utils.h"

// png magic bytes..
static const uint8_t magicNumber[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

int write_png_header(const pngenc_image_desc * desc,
                     pngenc_user_write_callback callback,
                     void* user_data) {

    // TODO: Make only one call to callback
    RETURN_ON_ERROR(callback(magicNumber, 8, user_data));

    // maps #channels to png constant
    const uint8_t channel_type[5] = {
        0, // Unused
        0, // GRAY
        4, // GRAY + ALPHA
        2, // RGB
        6  // RGBA
    };

    const png_header ihdr = {
        swap_endianness32(13),
        { 'I', 'H', 'D', 'R' },
        swap_endianness32(desc->width),
        swap_endianness32(desc->height),
        desc->bit_depth,
        channel_type[desc->num_channels],
        0, // compression method
        0, // filter method
        0  // interlace method
    };
    const uint32_t checksum = swap_endianness32(
                crc32c(0xffffffff, (const uint8_t*)ihdr.name, 4+13)^0xffffffff);
    RETURN_ON_ERROR(callback(&ihdr, 4+4+13, user_data));
    RETURN_ON_ERROR(callback(&checksum, 4, user_data));

    return PNGENC_SUCCESS;
}

int write_idat_block(const uint8_t * src, uint32_t len,
                     pngenc_user_write_callback callback,
                     void * user_data) {
    // Header + Data
    idat_header hdr = { swap_endianness32(len), { 'I', 'D', 'A', 'T' } };
    RETURN_ON_ERROR(callback(&hdr, sizeof(hdr), user_data));
    RETURN_ON_ERROR(callback(src, len, user_data));

    // Checksum
    uint32_t crc = crc32c(0xCA50F9E1, src, len);
    crc = swap_endianness32(crc ^ 0xFFFFFFFF);
    RETURN_ON_ERROR(callback(&crc, sizeof(uint32_t), user_data));

    return PNGENC_SUCCESS;
}

int write_png_end(pngenc_user_write_callback callback,
                  void * user_data) {
    png_end iend = { 0, { 'I', 'E', 'N', 'D' }, 0 };
    iend.check_sum = swap_endianness32(
                crc32c(0xffffffff, (uint8_t*)iend.name, 4) ^ 0xffffffff);
    return callback(&iend, sizeof(iend), user_data);
}

int read_png_header(pngenc_image_desc * desc, FILE * file) {
    uint8_t magic[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    fread(magic, 1, 8, file);
    if(memcmp(magic, magicNumber, 8) != 0) {
        return PNGENC_ERROR_NOT_A_PNG;
    }
    png_header ihdr;
    memset(&ihdr, 0, sizeof(png_header));
    fread(&ihdr, 4+4+13, 1, file);
    if(swap_endianness32(13) != ihdr.size
            || memcmp(ihdr.name, "IHDR", 4) != 0) {
        return PNGENC_ERROR_NOT_A_PNG;
    }
    desc->width = swap_endianness32(ihdr.width);
    desc->height = swap_endianness32(ihdr.height);
    desc->bit_depth = ihdr.bpp;
    if(ihdr.bpp != 8 && ihdr.bpp != 16) {
        return PNGENC_ERROR_UNSUPPORTED;
    }
    switch(ihdr.channel_type) {
        case 0: // GRAY
            desc->num_channels = 1;
            break;
        case 4: // GRAY + ALPHA
            desc->num_channels = 2;
            break;
        case 2: // RGB
            desc->num_channels = 3;
            break;
        case 6: // RGBA
            desc->num_channels = 4;
            break;
        default:
            return PNGENC_ERROR_UNSUPPORTED;
    }
    // TODO: add interlace support
    if(ihdr.compression_method | ihdr.filter_method | ihdr.interlace_mode) {
        return PNGENC_ERROR_UNSUPPORTED;
    }
    uint32_t checksum = 0;
    fread(&checksum, sizeof(checksum), 1, file);
    if(checksum != swap_endianness32(
                crc32c(0xffffffff, (uint8_t*)ihdr.name, 4+13)^0xffffffff)) {
        return PNGENC_ERROR_INVALID_CHECKSUM;
    }
    return PNGENC_SUCCESS;
}

int read_png_chunk_header(png_chunk * chunk, FILE * file) {
    size_t elements_read = fread(chunk, sizeof(png_chunk), 1, file);
    chunk->size = swap_endianness32(chunk->size);
    return elements_read == 1 ? PNGENC_SUCCESS : PNGENC_ERROR_FILE_IO;
}
