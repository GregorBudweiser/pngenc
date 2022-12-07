#include "common.h"
#include <string.h>
#include "../source/pngenc/adler32.h"
#include "../source/pngenc/crc32.h"
#include "../source/pngenc/huffman.h"

int perf_adler(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    init_cpu_info();

    const uint32_t M = 1024 * 1024;
    const uint32_t N = 64;
    uint8_t * src = (uint8_t*)malloc(N*M);
    uint8_t * dst = (uint8_t*)malloc(N*M);

    memset(src, 0x88, N*M);
    memset(dst, 0x88, N*M);

    int i;
    for(i = 0; i < (int)(N*M); i++) {
        src[i] = (uint8_t)rand();
    }

    printf("Memcpy:\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        memcpy(dst, src, N*M);
        TIMING_END_MB(N);
    }

    printf("Adler32:\n");
    for(i = 0; i < 5; i++) {
        uint32_t adler32 = 1;
        TIMING_START;
        adler_update(adler32, src, N*M);
        TIMING_END_MB(N);
    }

#if PNGENC_X86
    printf("Adler32_hw:\n");
    for(i = 0; i < 5; i++) {
        uint32_t adler32 = 1;
        TIMING_START;
        adler_update_hw(adler32, src, N*M);
        TIMING_END_MB(N);
    }
#endif

    printf("Histogram:\n");
    for(i = 0; i < 5; i++) {
        huffman_encoder enc;
        huffman_encoder_init(&enc);
        TIMING_START;
        huffman_encoder_add64(enc.histogram, src, N*M);
        TIMING_END_MB(N);
    }

    printf("Crc32:\n");
    for(i = 0; i < 5; i++) {
        uint32_t crc = 0xFFFFFFFF;
        TIMING_START;
        crc32(crc, src, N*M);
        TIMING_END_MB(N);
    }

    return 0;
}

