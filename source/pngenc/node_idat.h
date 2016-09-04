#include "node.h"

typedef struct _pngenc_node_idat {
    pngenc_node base;
    uint32_t crc;
} pngenc_node_idat;

int node_idat_init(pngenc_node_idat * node);
void node_destroy_idat(pngenc_node_idat * node);
