#include "node_data_gen.h"
#include "utils.h"
#include "malloc.h"

int64_t write_data_generator(struct _pngenc_node * node, const uint8_t * data,
                             uint32_t size);
int64_t finish_data_generator(struct _pngenc_node * node);
int64_t init_data_generator(struct _pngenc_node * node);


int node_data_generator_init(pngenc_node_data_gen * node,
                               const pngenc_image_desc * image) {
    node->base.buf = malloc(image->width*image->num_channels);
    node->base.buf_size = (uint64_t)image->width*(uint64_t)image->num_channels;
    node->base.buf_pos = 0;
    node->base.init = &init_data_generator;
    node->base.write = &write_data_generator;
    node->base.finish = &finish_data_generator;
    node->image = image;
    return PNGENC_SUCCESS;
}

int64_t write_data_generator(struct _pngenc_node * n, const uint8_t * data,
                             uint32_t size) {
    (void)data;
    (void)size;

    pngenc_node_data_gen * node = (pngenc_node_data_gen*)n;

    const pngenc_image_desc * image = node->image;
    const uint8_t row_filter = image->strategy == PNGENC_NO_COMPRESSION ? 0 : 1;

    uint64_t y;
    for(y = 0; y < image->height; y++) {
        RETURN_ON_ERROR(node_write(node->base.next, &row_filter, 1));
        if(row_filter == 0) {
            RETURN_ON_ERROR(node_write(node->base.next,
                                       image->data + y*image->row_stride,
                                       node->base.buf_size));
        } else {
            // delta-x row filter
            const uint8_t c = image->num_channels;
            const uint8_t * src = image->data + y*image->row_stride;
            const uint64_t length = node->base.buf_size;
            uint8_t * dst = (uint8_t*)node->base.buf;
            uint64_t i;
            for(i = 0; i < c; i++)
                dst[i] = src[i];

            for(i = c; i < length; i++)
                dst[i] = src[i] - src[i-c];

            RETURN_ON_ERROR(node_write(node->base.next, node->base.buf,
                                       image->width*image->num_channels));
        }
    }

    return (int64_t)image->height *
           ((int64_t)image->width*(int64_t)image->num_channels + 1L);
}

int64_t finish_data_generator(struct _pngenc_node * node) {
    (void)node;
    return PNGENC_SUCCESS;
}

int64_t init_data_generator(struct _pngenc_node * node) {
    (void)node;
    return PNGENC_SUCCESS;
}

void node_destroy_data_generator(pngenc_node_data_gen *node) {
    free(node->base.buf);
}
