#include <malloc.h>
#include "node_data_gen.h"
#include "utils.h"
#include "image_descriptor.h"

int64_t write_data_generator(struct _pngenc_node * node, const uint8_t * data,
                             uint32_t size);
int64_t finish_data_generator(struct _pngenc_node * node);
int64_t init_data_generator(struct _pngenc_node * node);


int node_data_generator_init(pngenc_node_data_gen * node,
                             const pngenc_image_desc * image) {
    node->base.buf_size = get_num_bytes_per_row(image);
    node->base.buf = malloc(node->base.buf_size);
    node->base.buf_pos = 0;
    node->base.init = &init_data_generator;
    node->base.write = &write_data_generator;
    node->base.finish = &finish_data_generator;
    node->image = image;
    return PNGENC_SUCCESS;
}

uint16_t swap_uint16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

int64_t write_data_generator(struct _pngenc_node * n, const uint8_t * data,
                             uint32_t size) {
    UNUSED(data);
    UNUSED(size);

    pngenc_node_data_gen * node = (pngenc_node_data_gen*)n;

    const pngenc_image_desc * image = node->image;
    const uint8_t row_filter = image->strategy == PNGENC_NO_COMPRESSION ? 0 : 1;

    uint64_t y;
    for(y = 0; y < image->height; y++) {
        RETURN_ON_ERROR(node_write(node->base.next, &row_filter, 1));
        if(image->bit_depth == 8) {
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
                                           node->base.buf_size));
            }
        } else { // 16bit
            // delta-x row filter
            const uint64_t c = image->num_channels;
            const uint16_t * src = (uint16_t*)(image->data + y*image->row_stride);
            const uint64_t length = node->base.buf_size/2;
            uint16_t * dst = (uint16_t*)node->base.buf;
            uint64_t i;

            if(row_filter == 0) {
                for(i = 0; i < length; i++)
                    dst[i] = swap_uint16(src[i]);
            } else {
                for(i = 0; i < c; i++)
                    dst[i] = swap_uint16(src[i]);

                for(i = c; i < length; i++)
                    dst[i] = swap_uint16(src[i]) - swap_uint16(src[i-c]);
            }

            RETURN_ON_ERROR(node_write(node->base.next, node->base.buf,
                                       node->base.buf_size));
        }
    }

    return (int64_t)image->height * (get_num_bytes_per_row(image) + 1LL);
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
