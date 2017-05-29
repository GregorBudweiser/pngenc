#include "common.h"
#include "string.h"
#include "../source/pngenc/node_deflate.h"

int64_t write_dbg(struct _pngenc_node * node, const uint8_t * data,
                  uint32_t size) {
    printf("got %d bytes\n", size);
    for(uint32_t i = 0; i < size; i++) {
        printf("(byte)% 4d%s", (int)data[i], (i & 0xF) == 0xF ? ",\n" : ", ");
        //printf("% 4d%s", (int)data[i], (i & 0xF) == 0xF ? ",\n" : ", ");
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

int unit_deflate(int argc, char* argv[]) {
    pngenc_node node_debug;
    node_debug.next = 0;
    node_debug.write = &write_dbg;
    node_debug.init = &init_dbg;
    node_debug.finish = &finish_dbg;
    node_init(&node_debug);

    pngenc_node_deflate node;
    RETURN_ON_ERROR(node_deflate_init(&node, PNGENC_FULL_COMPRESSION));

    node.base.next = &node_debug;

    uint8_t buf[100];
    memset(buf, 0xFF, 100);
    //for(int i = 0; i < 10; i++) {
    //    buf[10*i] = 0xFF;
    //}
    buf[11] = 0;//0xFF;
    RETURN_ON_ERROR(node.base.init(&node.base));
    RETURN_ON_ERROR(node_write(&node.base, buf, 13));
    RETURN_ON_ERROR(node_finish(&node.base));

    return PNGENC_SUCCESS;
}

