#pragma once
#include "utils.h"
#include "node.h"
#include "node_custom.h"
#include "node_data_gen.h"
#include "node_deflate.h"
#include "node_idat.h"

struct _pngenc_pipeline {
    pngenc_node_data_gen node_data_gen;
    pngenc_node_deflate node_deflate;
    pngenc_node_idat node_idat;
    pngenc_node_custom node_user;
};

int png_encoder_pipeline_destroy(pngenc_pipeline pipeline);

int png_encoder_pipeline_init(pngenc_pipeline pipeline,
                              const pngenc_image_desc * descriptor,
                              pngenc_user_write_callback callback,
                              void *user_data);

int png_encoder_pipeline_write(pngenc_pipeline pipeline,
                               const pngenc_image_desc * descriptor,
                               void * user_data);
