#pragma once
#include "node.h"

typedef struct _pngenc_node_custom {
    pngenc_node base;
    void * custom_data0;
    void * custom_data1;
    void * custom_data2;
    void * custom_data3;
    void * custom_data4;
    void * custom_data5;
} pngenc_node_custom;
