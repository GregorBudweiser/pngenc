#include "common.h"
#include <math.h>

int misc_pow(int argc, char* argv[]) {

    for(int i = 100; i < 1000; i++) {
        printf("%d => %d\n", i, (int)pow(i, 0.8));
    }

    // TODO: Make the test work
    return -1;
}

