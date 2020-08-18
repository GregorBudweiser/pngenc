#pragma once
#include "pngenc.h"
#include "zlib.h"
#include <stdint.h>
#include <stdio.h>


struct _pngenc_decoder {
    struct _zlib_codec zlib;
};

int pngenc_decode(pngenc_decoder decoder,
                  pngenc_image_desc * descriptor,
                  FILE * file);

void pngenc_defilter(const pngenc_image_desc * descriptor,
                     const uint8_t * zlib_output_buf);

uint8_t paeth_predictor(uint32_t a, uint32_t b, uint32_t c) ;
//uint8_t paeth_predictor(uint8_t a, uint8_t b, uint8_t c);
