#include <string.h>
#include "common.h"
#include "../source/pngenc/huffman.h"

int test_huffman_hist() {
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);

    const int N = 1024;
    uint8_t buf[N];
    memset(buf, 0, N);
    buf[0] = 1;
    buf[10] = 1;
    buf[100] = 1;

    huffman_encoder_add(&encoder, buf, N);

    ASSERT_TRUE(encoder.histogram[0] == N - 3);
    ASSERT_TRUE(encoder.histogram[1] == 3);
    for(int i = 2; i < 257; i++) {
        ASSERT_TRUE(encoder.histogram[i] == 0);
    }
    return 0;
}

int unit_huffman(int argc, char* argv[])
{
    (void)argc;
    (void*)argv;

    RETURN_ON_ERROR(test_huffman_hist());
    // TODO: Handle all the other stuff..
    return 0;
}
