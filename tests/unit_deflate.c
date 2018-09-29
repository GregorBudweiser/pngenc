#include "common.h"
#include "string.h"
#include "../source/pngenc/node_deflate.h"

#define N 64

int64_t write_dbg(struct _pngenc_node * node, const uint8_t * data,
                  uint32_t size) {
    printf("got %d bytes\n", size);
    for(uint32_t i = 0; i < size; i++) {
        printf("% 4d%s", (int)data[i], (i & 0xF) == 0xF ? ",\n" : ", ");
    }
    printf("\n");
    return size;
}

int64_t init_dbg(struct _pngenc_node * node) {
    return 0;
}

int64_t finish_dbg(struct _pngenc_node * node) {
    return 0;
}

int run_deflate(pngenc_compression_strategy strategy) {
    pngenc_node node_debug;
    node_debug.next = 0;
    node_debug.write = &write_dbg;
    node_debug.init = &init_dbg;
    node_debug.finish = &finish_dbg;
    RETURN_ON_ERROR(node_init(&node_debug));

    pngenc_node_deflate node_deflate;
    RETURN_ON_ERROR(node_deflate_init(&node_deflate, strategy));
    node_deflate.base.next = &node_debug;

    uint8_t buf[N];
    memset(buf, 0xFF, N);
    buf[11] = 0;

    RETURN_ON_ERROR(node_deflate.base.init(&node_deflate.base));
    RETURN_ON_ERROR(node_write(&node_deflate.base, buf, N));
    RETURN_ON_ERROR(node_finish(&node_deflate.base));

    node_destroy_deflate(&node_deflate);

    return PNGENC_SUCCESS;
}

int unit_deflate(int argc, char* argv[]) {
    RETURN_ON_ERROR(run_deflate(PNGENC_NO_COMPRESSION));
    RETURN_ON_ERROR(run_deflate(PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1));
    RETURN_ON_ERROR(run_deflate(PNGENC_FULL_COMPRESSION));
    return PNGENC_SUCCESS;
}

