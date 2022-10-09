#include "common.h"
#include "../source/pngenc/crc32.h"

int test_crc_sw() {
    crc32_init_sw();
    uint8_t data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    uint32_t c = crc32_sw(0xFFFFFFFF, data, sizeof(data))^0xFFFFFFFF;
    ASSERT_TRUE(c == 0x3fca88c5);
    return 0;
}

int test_crc_hw() {
    const size_t OFFSET = 5;
    const size_t N = 1024*1024;
    uint8_t * data = malloc(N+64+OFFSET);
    data += OFFSET;
    memset(data, 0x8, N);

    uint32_t sw = crc32_sw(0xFFFFFFFF, data, N)^0xFFFFFFFF;
    uint32_t hw = crc32(0xFFFFFFFF, data, N)^0xFFFFFFFF;
    ASSERT_TRUE(sw == hw);

    free(data - OFFSET);
    return 0;
}

int unit_crc(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    RETURN_ON_ERROR(test_crc_sw());
    RETURN_ON_ERROR(test_crc_hw());
    return 0;
}

