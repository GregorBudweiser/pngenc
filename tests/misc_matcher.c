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

    uint16_t hist[32768];
    memset(hist, 0, 32768*sizeof(uint16_t));
    TIMING_START;
    RETURN_ON_ERROR(histogram(buf, W*H*C, hist));
    TIMING_END;

    for (int i = 0; i < 100; i++) {
        printf("Match for %d: %d\n", i, (int)hist[i]);
    }

    free(buf);
    return 0;
}

