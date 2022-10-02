#include <string.h>
#include <malloc.h>
#include "common.h"
#include "../source/pngenc/adler32.h"

int test_adler_single_call() {
    uint32_t adler = 1;

    // Compute adler crc for wikipedia
    const char buf[9] = { 'W', 'i', 'k', 'i', 'p', 'e', 'd', 'i', 'a' };
    adler = adler_update(adler, (const uint8_t*)buf, 9);

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
    uint32_t adler = 1;

    // 1M buffer
    const int N = 1024*1024;
    uint8_t * buf = (uint8_t*)malloc(N);
    memset(buf, 0, N);
    adler = adler_update(adler, buf, N);

    ASSERT_TRUE((adler & 0xFFFF) == 1);
    ASSERT_TRUE((adler >> 16) == 240);
    free(buf);
    return 0;
}

int unit_adler(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    RETURN_ON_ERROR(test_adler_single_call());
    RETURN_ON_ERROR(test_adler_multi_call());
    RETURN_ON_ERROR(test_adler_1M_buffer());
    return 0;
}
