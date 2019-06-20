#include "common.h"
#include "../source/pngenc/crc32.h"

int unit_crc(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    uint8_t buffer[4] = { 'I', 'D', 'A', 'T' };
    uint32_t crc = crc32c(0xFFFFFFFF, buffer, 4);
    ASSERT_TRUE(crc == 0xCA50F9E1);
    return 0;
}

