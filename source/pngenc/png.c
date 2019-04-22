#include "png.h"
#include <string.h>
#include "crc32.h"
#include "utils.h"

int write_png_header(const pngenc_image_desc * desc,
                     pngenc_user_write_callback callback,
                     void* user_data) {

    // TODO: Make only one call to callback
    // TODO: Handle callback code (easy once we only do one call)

    // png magic bytes..
    const uint8_t magicNumber[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    callback(magicNumber, 8, user_data);

    // maps #channels to png constant
    const uint8_t channel_type[5] = {
        0, // Unused
        0, // GRAY
        4, // GRAY + ALPHA
        2, // RGB
        6  // RGBA
    };

    png_header ihdr = {
        swap_endianness32(13),
        { 'I', 'H', 'D', 'R' },
        swap_endianness32(desc->width),
        swap_endianness32(desc->height),
        desc->bit_depth,
        channel_type[desc->num_channels],
        0, // filter is "sub" or "none"
        0, // filter
        0  // no interlacing supported currently
    };
    uint32_t check_sum = swap_endianness32(
                crc32c(0xffffffff, (uint8_t*)ihdr.name, 4+13)^0xffffffff);
    callback(&ihdr, 4+4+13, user_data);
    callback(&check_sum, 4, user_data);

    return 0;
}

int write_idat_block(const uint8_t * src, uint32_t len,
                     pngenc_user_write_callback callback,
                     void * user_data) {
    // Header + Data
    idat_header hdr = { len, { 'I', 'D', 'A', 'T' } };
    callback(&hdr, sizeof(hdr), user_data);
    callback(&src, len, user_data);

    // Checksum
    uint32_t crc = crc32c(0xCA50F9E1, src, len);
    crc = swap_endianness32(crc ^ 0xFFFFFFFF);
    callback(&crc, sizeof(uint32_t), user_data);

    return 0; // TODO: handle callback return codes..
}

int write_png_end(pngenc_user_write_callback callback,
                  void * user_data) {
    png_end iend = { 0, { 'I', 'E', 'N', 'D' }, 0 };
    iend.check_sum = swap_endianness32(
                crc32c(0xffffffff, (uint8_t*)iend.name, 4) ^ 0xffffffff);
    callback(&iend, sizeof(iend), user_data);
    return 0;
}
