#include "common.h"
#include "../source/pngenc/matcher.h"
#include "../source/pngenc/huffman.h"
#include "string.h"

int perf_huffman(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    const int W = 1920;
    const int H = 1080;
    const int C = 3;
    uint8_t * buf = (uint8_t*)malloc(W*H*C);
    {
        FILE * f = fopen("data.bin", "rb");
        if(f == 0)
            return -1;
        fread(buf, C, W*H, f);
        fclose(f);
    }

    huffman_encoder encoder;
    huffman_encoder_init(&encoder);

    int i, j;
    printf("skimm through memory\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        const int end = C*W*H/8;
        for(j = 0; j < end; j++) {
            volatile register uint64_t tmp = ((uint64_t*)buf)[j];
            UNUSED(tmp);
        }
        TIMING_END;
    }

    printf("optimized\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        huffman_encoder_add(encoder.histogram, buf, C*W*H);
        TIMING_END;
    }

    printf("simple\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        huffman_encoder_add_simple(encoder.histogram, buf, C*W*H);
        TIMING_END;
    }

    free(buf);
    return 0;
}

