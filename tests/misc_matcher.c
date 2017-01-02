#include "common.h"
#include "../source/pngenc/matcher.h"
#include "../source/pngenc/huffman.h"
#include <math.h>
#include <malloc.h>
#include <string.h>

int misc_matcher(int argc, char* argv[]) {

    UNUSED(argc);
    UNUSED(argv);

    const int W = 1920;
    const int H = 1080;
    const int C = 3;
    uint8_t * buf = (uint8_t*)malloc(W*H*C);
    {
        FILE * f = fopen("data.bin", "rb");
        if(f == 0)
            return 0;
        fread(buf, C, W*H, f);
        fclose(f);
    }

    uint16_t * out = (uint16_t*)malloc(W*H*C*sizeof(uint16_t));

    huffman_encoder encoder_hist;
    huffman_encoder encoder_dist;
    huffman_encoder_init(&encoder_hist);
    huffman_encoder_init(&encoder_dist);

    uint32_t out_length = 0;
    {
        TIMING_START;
        RETURN_ON_ERROR(histogram(buf, W*H*C, encoder_hist.histogram,
                                  encoder_dist.histogram, out, &out_length));
        TIMING_END;
    }

    {
        TIMING_START;
        RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder_hist, 15));
        TIMING_END;
    }

    {
        TIMING_START;
        RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder_dist, 15));
        TIMING_END;
    }

    {
        uint64_t offset = 0;
        TIMING_START;
        RETURN_ON_ERROR(huffman_encoder_encode_full_simple(&encoder_hist, &encoder_dist, out, out_length, buf, &offset));
        TIMING_END;
        printf("Out length: %d\n", (int)offset/8);
    }


    printf("out_length: %d/%d => %.02f%%\n",
           (int)out_length, C*W*H, 100.0f*(float)out_length/(float)(C*W*H));

    free(out);
    free(buf);
    return 0;
}
