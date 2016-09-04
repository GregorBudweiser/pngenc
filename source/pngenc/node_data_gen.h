#pragma once
#include "node.h"
#include "pngenc.h"

typedef struct _pngenc_node_data_gen {
    pngenc_node base;
    const pngenc_image_desc * image;
} pngenc_node_data_gen;

int node_data_generator_init(pngenc_node_data_gen * node,
                               const pngenc_image_desc * image);
void node_destroy_data_generator(pngenc_node_data_gen * node);

