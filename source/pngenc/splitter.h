#pragma once

#include "pngenc.h"

int split(const pngenc_encoder encoder, const pngenc_image_desc * desc,
          pngenc_user_write_callback callback, void * user_data);

pngenc_result write_png(const pngenc_encoder encoder,
                        const pngenc_image_desc * desc,
                        pngenc_user_write_callback callback, void *user_data);
