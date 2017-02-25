#include "png_pipeline.h"
#include "malloc.h"
#include "callback.h"
#include "node.h"
#include "node_custom.h"
#include "node_deflate.h"
#include "node_idat.h"
#include "node_data_gen.h"
#include "pngenc.h"

int png_encoder_pipeline_init(pngenc_pipeline pipeline,
                              const pngenc_image_desc * descriptor,
                              pngenc_user_write_callback callback,
                              void * user_data) {

    node_data_generator_init(&pipeline->node_data_gen, descriptor);
    node_idat_init(&pipeline->node_idat);
    node_deflate_init(&pipeline->node_deflate, descriptor->strategy);

    pipeline->node_user.base.next = 0;
    pipeline->node_user.base.init = 0;
    pipeline->node_user.base.write = &user_write_wrapper;
    pipeline->node_user.base.finish = 0;
    pipeline->node_user.custom_data0 = user_data;
    pipeline->node_user.custom_data1 = callback;

    pipeline->node_data_gen.base.next = (pngenc_node*)&pipeline->node_idat;
    pipeline->node_idat.base.next = (pngenc_node*)&pipeline->node_deflate;
    pipeline->node_deflate.base.next = (pngenc_node*)&pipeline->node_user;

    return PNGENC_SUCCESS;
}

int png_encoder_pipeline_write(pngenc_pipeline pipeline,
                               const pngenc_image_desc * descriptor,
                               void * user_data) {
    //RETURN_ON_ERROR(node_data_generator_init()) // TODO: update source..
    RETURN_ON_ERROR(node_write((pngenc_node*)&pipeline->node_data_gen, 0, 1));
    RETURN_ON_ERROR(node_finish((pngenc_node*)&pipeline->node_data_gen));
    return PNGENC_SUCCESS;

}

int png_encoder_pipeline_destroy(pngenc_pipeline pipeline) {
    node_destroy_data_generator(&pipeline->node_data_gen);
    node_destroy_deflate(&pipeline->node_deflate);
    node_destroy_idat(&pipeline->node_idat);
    return 0;
}
