#include "common.h"
#include "../source/pngenc/matcher.h"
#include <math.h>
#include <malloc.h>

#define MATCH_BUF_SIZE (1024*1024)

int misc_matcher(int argc, char* argv[]) {

    uint8_t * buf = malloc(MATCH_BUF_SIZE);
    uint32_t i;
    for(i = 0; i < MATCH_BUF_SIZE; i++) {
        buf[i] = rand() % 4;
    }

    uint16_t hist[32768];
    memset(hist, 0, 32768*sizeof(uint16_t));
    RETURN_ON_ERROR(histogram(buf, MATCH_BUF_SIZE, hist));

    for (int i = 0; i < 100; i++) {
        printf("Match for %d: %d\n", i, (int)hist[i]);
    }

    free(buf);
    return 0;
}

