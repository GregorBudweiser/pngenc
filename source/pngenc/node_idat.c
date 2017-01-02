#include "utils.h"
#include "node_idat.h"
#include "crc32.h"
#include "pngenc.h"
#include <string.h>
#include <assert.h>

int64_t write_idat(struct _pngenc_node * node, const uint8_t * data,
                   uint32_t size);
int64_t finish_idat(struct _pngenc_node * node);
int64_t init_idat(struct _pngenc_node * node);

int node_idat_init(pngenc_node_idat * node) {
    node->base.buf_size = 129*1024; // should roughly fit two zlib blocks
    node->base.buf = malloc(node->base.buf_size);
    node->base.buf_pos = 0;
    node->base.next = 0;

    // init crc32
    node->crc = 0xCA50F9E1;

    // set functions
    node->base.finish = &finish_idat;
    node->base.init = &init_idat;
    node->base.write = &write_idat;
    return PNGENC_SUCCESS;
}

void node_destroy_idat(pngenc_node_idat * node) {
    free(node->base.buf);
}

int64_t write_idat(struct _pngenc_node * n, const uint8_t * data,
                   uint32_t size) {

    pngenc_node_idat * node = (pngenc_node_idat*)n;
    assert(node->base.buf_pos <= node->base.buf_size);

    // buffer full?
    if(node->base.buf_pos == node->base.buf_size) {
        struct idat_header {
            uint32_t size;
            uint8_t name[4];
        } hdr = {
            swap_endianness32((uint32_t)node->base.buf_size),
            { 'I', 'D', 'A', 'T' }
        };

        // write another zlib block
        RETURN_ON_ERROR(node_write(node->base.next, (uint8_t*)&hdr, 8));
        RETURN_ON_ERROR(node_write(node->base.next, node->base.buf,
                                   node->base.buf_size));

        // finally write the checksum for this block
        uint32_t crc32 = node->crc ^ 0xFFFFFFFF;
        crc32 = swap_endianness32(crc32);
        RETURN_ON_ERROR(node_write(node->base.next, (uint8_t*)&crc32, 4));

        // reset crc for next block
        node->crc = 0xCA50F9E1;

        node->base.buf_pos = 0;
    }

    // consume as much of current data as possible
    int64_t bytes_remaining = node->base.buf_size - node->base.buf_pos;
    int64_t bytes_written = min_i64(bytes_remaining, size);
    memcpy((uint8_t*)node->base.buf + node->base.buf_pos, data, bytes_written);
    node->crc = crc32c(node->crc, (uint8_t*)node->base.buf + node->base.buf_pos,
                       bytes_written);
    node->base.buf_pos += bytes_written;
    return bytes_written;
}

int64_t finish_idat(struct _pngenc_node * n) {
    pngenc_node_idat * node = (pngenc_node_idat*)n;
    if(node->base.buf_pos > 0) {
        struct idat_header {
            uint32_t size;
            uint8_t name[4];
        } hdr = {
            swap_endianness32((uint32_t)node->base.buf_pos),
            { 'I', 'D', 'A', 'T' }
        };

        // write another zlib block
        RETURN_ON_ERROR(node_write(node->base.next, (uint8_t*)&hdr, 8));
        RETURN_ON_ERROR(node_write(node->base.next, node->base.buf,
                                   node->base.buf_pos));


        // finally write the checksum
        uint32_t crc32 = node->crc ^ 0xFFFFFFFF;
        crc32 = swap_endianness32(crc32);
        RETURN_ON_ERROR(node_write(node->base.next, (uint8_t*)&crc32, 4));

        node->base.buf_pos = 0;
    }

    // .. and we're done!
    return PNGENC_SUCCESS;
}

int64_t init_idat(struct _pngenc_node * node) {
    UNUSED(node);
    return PNGENC_SUCCESS;
}
