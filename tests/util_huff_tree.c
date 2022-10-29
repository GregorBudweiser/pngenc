#include "common.h"
#include "../source/pngenc/huffman.h"

int util_huff_tree(int argc, char* argv[]) {
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);

    encoder.histogram[0] = 1;
    encoder.histogram[1] = 1;
    encoder.histogram[2] = 2;
    encoder.histogram[3] = 3;
    encoder.histogram[4] = 5;
    encoder.histogram[5] = 8;
    encoder.histogram[6] = 13;
    encoder.histogram[7] = 14;
    huffman_encoder_build_tree_limited(&encoder, 3);
    huffman_encoder_print(&encoder, "distances");

    return 0;
}

