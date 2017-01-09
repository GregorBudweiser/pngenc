#pragma once
#include <stdint.h>
#include "node.h"
#include "pngenc.h"
#include "huffman.h"
#include "adler32.h"

typedef struct _pngenc_node_deflate {
    pngenc_node base;
    pngenc_adler32 adler;
    uint8_t * compressed_buf;
    uint64_t bit_offset;
    pngenc_compression_strategy strategy;
} pngenc_node_deflate;

int node_deflate_init(pngenc_node_deflate * node,
                      const pngenc_image_desc * image);
void node_destroy_deflate(pngenc_node_deflate * node);
