#include "common.h"
#include "../source/pngenc/matcher.h"
#include "../source/pngenc/huffman.h"

int perf_huffman(int argc, char* argv[]) {
    (void)argc;
    (void*)argv;

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

    huffman_encoder encoder_hist;
    huffman_encoder_init(&encoder_hist);

    int i;
    for(i = 0; i < 5; i++) {
        TIMING_START;
        RETURN_ON_ERROR(huffman_encoder_add(&encoder_hist, buf, C*W*H));
        TIMING_END;
    }

    for(i = 0; i < 5; i++) {
        TIMING_START;
        RETURN_ON_ERROR(huffman_encoder_add_simple(&encoder_hist, buf, C*W*H));
        TIMING_END;
    }

    free(buf);
    return 0;
}

