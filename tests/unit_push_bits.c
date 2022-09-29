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

int test_push_bits_a() {
    uint8_t buf[16];
    memset(buf, 0, sizeof(buf));
    uint64_t bit_offset = 0;

    // set 6 bytes
    bit_offset = push_bits_a(0xFFFFFFFFFFFFul, 48, buf, bit_offset);
    ASSERT_TRUE(bit_offset == 48);
    for (int i = 0; i < 16; i++) {
        ASSERT_TRUE(buf[i] == ((i < 6) ? 0xFF : 0));
    }

    // set another 6 bytes
    bit_offset = push_bits_a(0xFFFFFFFFFFFFul, 48, buf, bit_offset);
    for (int i = 0; i < 16; i++) {
        ASSERT_TRUE(buf[i] == ((i < 12) ? 0xFF : 0));
    }

    return 0;
}

int test_push_bits_a_one_bit() {
    uint8_t buf[16];
    memset(buf, 0, sizeof(buf));
    uint64_t bit_offset = 0;

    for (int i = 0; i < 128; i++) {
        bit_offset = push_bits_a(1, 1, buf, bit_offset);
    }

    for (int i = 0; i < 16; i++) {
        ASSERT_TRUE(buf[i] == 0xFF);
    }

    return 0;
}


int unit_push_bits(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    RETURN_ON_ERROR(test_push_bits());
    RETURN_ON_ERROR(test_push_bits_a());
    RETURN_ON_ERROR(test_push_bits_a_one_bit());
    return 0;
}

