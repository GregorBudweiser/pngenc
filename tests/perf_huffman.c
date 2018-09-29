#include "common.h"
#include "../source/pngenc/matcher.h"
#include "../source/pngenc/huffman.h"
#include "string.h"

const int W = 1920;
const int H = 1080;
const int C = 3;

void perf_add(const uint8_t * buf) {
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
}

void perf_encode(const uint8_t * buf, uint8_t * dst) {
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);
    huffman_encoder_add(encoder.histogram, buf, C*W*H);
    huffman_encoder_build_tree_limited(&encoder, 15, 0.95);

    uint64_t offset;
    int i;

    printf("encode_simple\n");
    for(i = 0; i < 5; i++) {
        offset = 0;
        TIMING_START;
        memset(dst, 0, W*H*C);
        huffman_encoder_encode_simple(&encoder, buf, C*W*H, dst, &offset);
        TIMING_END;
    }

    printf("encode64\n");
    for(i = 0; i < 5; i++) {
        offset = 0;
        TIMING_START;
        memset(dst, 0, W*H*C);
        huffman_encoder_encode64(&encoder, buf, C*W*H, dst, &offset);
        TIMING_END;
    }

    printf("encode64_2\n");
    for(i = 0; i < 5; i++) {
        offset = 0;
        TIMING_START;
        memset(dst, 0, W*H*C);
        huffman_encoder_encode64_2(&encoder, buf, C*W*H, dst, &offset);
        TIMING_END;
    }

    printf("encode64_3\n");
    for(i = 0; i < 5; i++) {
        offset = 0;
        TIMING_START;
        // Does not need memset
        huffman_encoder_encode64_3(&encoder, buf, C*W*H, dst, &offset);
        TIMING_END;
    }
}

int perf_huffman(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    uint8_t * buf = (uint8_t*)malloc(W*H*C);
    {
        FILE * f = fopen("data.bin", "rb");
        if(f == 0) {
            printf("Could not open file: data.bin\n");
            return -1;
        }
        fread(buf, C, W*H, f);
        fclose(f);
    }

    uint8_t * dst = (uint8_t*)malloc(W*H*C);

    perf_add(buf);
    perf_encode(buf, dst);

    free(buf);
    return 0;
}

