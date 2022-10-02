#include "common.h"
#include "../source/pngenc/huffman.h"
#include "string.h"

static const int W = 1920;
static const int H = 1080;
static const int C = 3;

void perf_add(const uint8_t * src, uint8_t * dst) {
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);

    int i, j;
    printf("skimm through memory\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        const int end = C*W*H/8;
        for(j = 0; j < end; j++) {
            volatile register uint64_t tmp = ((uint64_t*)src)[j];
            UNUSED(tmp);
        }
        TIMING_END_MB(6);
    }

    printf("optimized\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        huffman_encoder_add(encoder.histogram, src, C*W*H);
        TIMING_END_MB((double)(W*H*C)/1e6);
    }

    printf("simple\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        huffman_encoder_add_simple(encoder.histogram, src, C*W*H);
        TIMING_END_MB((double)(W*H*C)/1e6);
    }

    printf("memset\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        memset(dst, 0, C*W*H);
        TIMING_END_MB((double)(W*H*C)/1e6);
    }
}

int perf_encode(const uint8_t * buf, uint8_t * dst) {
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);
    huffman_encoder_add(encoder.histogram, buf, C*W*H);
    huffman_encoder_build_tree_limited(&encoder, 15, 0.8);
    huffman_encoder_build_codes_from_lengths(&encoder);

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

    printf("encode64_3\n");
    for(i = 0; i < 5; i++) {
        offset = 0;
        TIMING_START;
        // Does not need memset
        huffman_encoder_encode64_3(&encoder, buf, C*W*H, dst, &offset);
        TIMING_END;
    }

    return 0;
}

int perf_huffman(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    uint8_t * src;
    uint8_t * dst;
    {
        FILE * file = fopen("data.bin", "rb");
        if(file == 0) {
            printf("Could not find raw image data input.\n");
            printf("Please provide binary image data in the file 'data.bin'.\n");
            printf("Expecting 1920x1080x3 bytes of data.\n");
            return 0;
        }
        src = (uint8_t*)malloc(W*H*C);
        dst = (uint8_t*)malloc(W*H*C);
        fread(src, C, W*H, file);
        fclose(file);
    }

    perf_add(src, dst);
    RETURN_ON_ERROR(perf_encode(src, dst));

    free(src);
    free(dst);
    return 0;
}

