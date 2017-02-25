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

    uint32_t i;
    for(i = W*H*C-1; i >= 3; i--) {
        buf[i] = buf[i] - buf[i-3];
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

    uint64_t offset = 0;
    {
        TIMING_START;
        RETURN_ON_ERROR(huffman_encoder_encode_full_simple(&encoder_hist, &encoder_dist, out, out_length, buf, &offset));
        TIMING_END;
    }
    /*fflush(stdout);
    {
        TIMING_START;
        int i,j;
        uint32_t match_lenghts[250];
        for(i = 0; i < W*H*C; i++) {
            uint32_t best = 0;
            for(j = max_i32(0, i - 16000); j < i; j++) {
                uint32_t n = match_fwd(buf, 250, i, j);
                best = max_u32(best, n);
            }
            match_lenghts[min_u32(best, 250)];
        }
        TIMING_END;
    }*/

    /*{
        int i;
        float * data = malloc(W*H*C*sizeof(float));
        memset(data, 0, W*H*C*sizeof(float));
        TIMING_START;
        for(i = 0; i < W*H*C; i++) {
            data[i] = buf[i];
        }
        TIMING_END;
    }

    {
        int i;
        float * data = malloc(W*H*C*sizeof(float));
        memset(data, 0, W*H*C*sizeof(float));
        TIMING_START;
        //for(i = 0; i < 256; i++)
        for(i = 0; i < W*H*C; i++)
        {
            uint32_t value = (i << 15) | 0x3F800000;
            data[i] = (*(float*)(&value) - 1.0f)*256.0f;
            //printf("%d => %f\n", i, data[i]);
        }
        TIMING_END;
    }*/

    printf("out_length: %d/%d => %.02f%%\n",
           (int)offset/8, C*W*H, 100.0f*(float)(offset/8)/(float)(C*W*H));

    free(out);
    free(buf);
    return 0;
}
