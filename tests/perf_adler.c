#include "common.h"
#include <string.h>
#include "../source/pngenc/adler32.h"
#include "../source/pngenc/crc32.h"

int perf_adler(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    const uint32_t M = 1024 * 1024;
    const uint32_t N = 32;
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
    pngenc_adler32 adler32;
    for(i = 0; i < 5; i++) {
        adler_init(&adler32);
        TIMING_START;
        adler_update(&adler32, src, N*M);
        TIMING_END_MB(N);
    }

    printf("Crc32:\n");
    uint32_t crc = 0xFFFFFFFF;
    for(i = 0; i < 5; i++) {
        adler_init(&adler32);
        TIMING_START;
        crc32c(crc, src, N*M);
        TIMING_END_MB(N);
    }


    return 0;
}

