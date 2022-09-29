#include "common.h"
#include "../source/pngenc/huffman.h"

int util_huff_tree(int argc, char* argv[]) {
    huffman_encoder dist_encoder;
    huffman_encoder_init(&dist_encoder);
    dist_encoder.code_lengths[0] = 1;
    huffman_encoder_build_codes_from_lengths(&dist_encoder);

    huffman_encoder_print(&dist_encoder, "distances");
    return 0;
}

