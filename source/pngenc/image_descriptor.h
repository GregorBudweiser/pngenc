#pragma once

#include "pngenc.h"

/**
 * Get number of bytes for one scanline/row in the image given the descriptor.
 */
uint64_t get_num_bytes_per_row(const pngenc_image_desc * descriptor);

/**
 * Check the image descriptor for plausible values.
 * @returns Nonzero if invalid. Zero if plausible.
 */
int check_descriptor(const pngenc_image_desc * descriptor);
