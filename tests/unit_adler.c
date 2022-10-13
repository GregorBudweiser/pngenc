#include <string.h>
#include <malloc.h>
#include "common.h"
#include "../source/pngenc/adler32.h"

int test_adler_hw() {
    uint8_t buf[400];
    memset(buf, 0x01, sizeof(buf));

    const uint32_t hw = adler_update_hw(1, buf, sizeof(buf));
    const uint32_t sw = adler_update(1, buf, sizeof(buf));

    printf("0x%08x\n", sw);
    printf("0x%08x\n", hw);

    ASSERT_TRUE(hw == sw);
    return 0;
}

int test_adler_single_call() {
    uint32_t adler = 1;

    // Compute adler crc for wikipedia
    const char buf[9] = { 'W', 'i', 'k', 'i', 'p', 'e', 'd', 'i', 'a' };
    adler = adler_update(adler, (const uint8_t*)buf, sizeof(buf));

    // See: https://en.wikipedia.org/wiki/Adler-32#Example
    ASSERT_TRUE((adler & 0xFFFF) == 920);
    ASSERT_TRUE((adler >> 16) == 4582);
    return 0;
}

int test_adler_multi_call() {
    uint32_t adler = 1;

    // Compute adler crc for wikipedia
    const char buf[9] = { 'W', 'i', 'k', 'i', 'p', 'e', 'd', 'i', 'a' };
    adler = adler_update(adler, (const uint8_t*)buf, 4);
    adler = adler_update(adler, (const uint8_t*)buf+4, 5);

    // See: https://en.wikipedia.org/wiki/Adler-32#Example
    ASSERT_TRUE((adler & 0xFFFF) == 920);
    ASSERT_TRUE((adler >> 16) == 4582);
    return 0;
}

int test_adler_1M_buffer() {
    // 1M buffer
    const int N = 1024*1024;
    uint8_t * buf = (uint8_t*)malloc(N);
    memset(buf, 0, N);
    uint32_t adler = adler_update(1, buf, N);
    free(buf);

    ASSERT_TRUE((adler & 0xFFFF) == 1);
    ASSERT_TRUE((adler >> 16) == 240);
    return 0;
}

int unit_adler(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    RETURN_ON_ERROR(test_adler_hw());
    RETURN_ON_ERROR(test_adler_single_call());
    RETURN_ON_ERROR(test_adler_multi_call());
    RETURN_ON_ERROR(test_adler_1M_buffer());
    return 0;
}
