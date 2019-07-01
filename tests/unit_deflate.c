#include "common.h"
#include "string.h"

#include "../source/pngenc/huffman.h"
#include "../source/pngenc/deflate.h"

int test_push_bits() {
    uint8_t buf[2] = { 0, 0 };
    uint64_t bit_offset = 0;

    for(int i = 0; i < 5; i++) {
        push_bits(0x7, 3, buf, &bit_offset);
        printf("%u, %u\n", (uint32_t)buf[0], (uint32_t)buf[1]);
    }
    push_bits(0x1, 1, buf, &bit_offset);

    printf("%u\n", (uint32_t)buf[0]);
    printf("%u\n", (uint32_t)buf[1]);

    ASSERT_TRUE(buf[0] == 0xFF);
    ASSERT_TRUE(buf[1] == 0xFF);

    return 0;
}

int unit_deflate(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argc);
    RETURN_ON_ERROR(test_push_bits());

    return 0; // TODO
}

