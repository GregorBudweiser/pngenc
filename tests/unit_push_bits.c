#include "common.h"

#include "../source/pngenc/huffman.h"

int test_push_bits() {
    uint8_t buf[3] = { 0, 0, 0 };
    uint64_t bit_offset = 0;

    // put 5 times three bits into buffer/stream
    for(int i = 0; i < 5; i++) {
        push_bits(0x7, 3, buf, &bit_offset);
    }
    // put one more bit into buffer (for a total of 16 bits)
    push_bits(0x1, 1, buf, &bit_offset);

    // test if we have 16 bits set to one
    ASSERT_TRUE(buf[0] == 0xFF);
    ASSERT_TRUE(buf[1] == 0xFF);
    ASSERT_TRUE(buf[2] == 0x0);

    return 0;
}

int unit_push_bits(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    RETURN_ON_ERROR(test_push_bits());
    return 0;
}

