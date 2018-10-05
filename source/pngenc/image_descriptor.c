#include "image_descriptor.h"

uint64_t get_num_bytes_per_row(const pngenc_image_desc * descriptor) {
    return (uint64_t)descriptor->width * (uint64_t)descriptor->num_channels
         * (uint64_t)(descriptor->bit_depth/8);
}

int check_descriptor(const pngenc_image_desc * descriptor) {
    if(descriptor->data == 0)
        return PNGENC_ERROR_INVALID_ARG;

    if(descriptor->width == 0 || descriptor->height == 0)
        return PNGENC_ERROR_INVALID_ARG;

    if(descriptor->num_channels > 4 || descriptor->num_channels == 0)
        return PNGENC_ERROR_INVALID_ARG;

    if((uint64_t)descriptor->row_stride < get_num_bytes_per_row(descriptor))
        return PNGENC_ERROR_INVALID_ARG;

    if(descriptor->bit_depth != 8 && descriptor->bit_depth != 16)
        return PNGENC_ERROR_INVALID_ARG;

    return PNGENC_SUCCESS;
}
