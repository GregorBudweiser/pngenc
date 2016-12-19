#include "common.h"
#include "../source/pngenc/matcher.h"
#include <math.h>
#include <malloc.h>

int misc_matcher(int argc, char* argv[]) {

    (void)argc;
    (void*)argv;

    const int W = 1920;
    const int H = 1080;
    const int C = 3;
    uint8_t * buf = (uint8_t*)malloc(W*H*C);
    {
        FILE * f = fopen("data.bin", "rb");
        fread(buf, C, W*H, f);
        fclose(f);
    }

    uint16_t * out = (uint16_t*)malloc(W*H*C*sizeof(uint16_t));
    uint32_t hist[258];
    memset(hist, 0, 258*sizeof(uint16_t));
    uint32_t out_length = 0;
    TIMING_START;
    RETURN_ON_ERROR(histogram(buf, W*H*C, hist, out, &out_length));
    TIMING_END;

    printf("out_length: %d/%d\n", out_length, C*W*H);

    free(out);
    free(buf);
    return 0;
}

