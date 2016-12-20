#include "common.h"
#include "../source/pngenc/matcher.h"
#include <math.h>
#include <malloc.h>
#include <string.h>

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
    uint32_t * hist = (uint32_t*)malloc(285*sizeof(uint32_t));
    uint32_t * dist_hist = (uint32_t*)malloc(30*sizeof(uint32_t));
    memset(hist, 0, 285*sizeof(uint32_t));
    memset(dist_hist, 0, 30*sizeof(uint32_t));


    uint32_t out_length = 0;
    TIMING_START;
    RETURN_ON_ERROR(histogram(buf, W*H*C, hist, dist_hist, out, &out_length));
    TIMING_END;

    printf("out_length: %d/%d => %.02f%%\n", (int)out_length, C*W*H, 100.0f*(float)out_length/(float)(C*W*H));

    free(out);
    free(buf);
    return 0;
}
