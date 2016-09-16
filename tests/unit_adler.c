#include <string.h>
#include <malloc.h>
#include "common.h"
#include "../source/pngenc/adler32.h"

int test_init() {
    pngenc_adler32 adler;
    adler_init(&adler);

    ASSERT_TRUE(adler.s1 == 1);
    ASSERT_TRUE(adler.s2 == 0);
    return 0;
}

int test_get() {
    pngenc_adler32 adler;
    adler_init(&adler);
    ASSERT_TRUE(adler_get_checksum(&adler) == 1);
    return 0;
}

int test_adler_single_call() {
    pngenc_adler32 adler;
    adler_init(&adler);

    // Compute adler crc for wikipedia
    const char buf[9] = { 'W', 'i', 'k', 'i', 'p', 'e', 'd', 'i', 'a' };
    adler_update(&adler, buf, 9);

    // See: https://en.wikipedia.org/wiki/Adler-32#Example
    ASSERT_TRUE(adler.s1 == 920);
    ASSERT_TRUE(adler.s2 == 4582);
    return 0;
}

int test_adler_multi_call() {
    pngenc_adler32 adler;
    adler_init(&adler);

    // Compute adler crc for wikipedia
    const char buf[9] = { 'W', 'i', 'k', 'i', 'p', 'e', 'd', 'i', 'a' };
    adler_update(&adler, buf, 4);
    adler_update(&adler, buf+4, 5);

    // See: https://en.wikipedia.org/wiki/Adler-32#Example
    ASSERT_TRUE(adler.s1 == 920);
    ASSERT_TRUE(adler.s2 == 4582);
    return 0;
}

int test_adler_1M_buffer() {
    pngenc_adler32 adler;
    adler_init(&adler);

    // 1M buffer
    const int N = 1024*1024;
    char * buf = (char*)malloc(N);
    memset(buf, 0, N);
    adler_update(&adler, buf, N);

    ASSERT_TRUE(adler.s1 == 1);
    ASSERT_TRUE(adler.s2 == 240);

    free(buf);

    return 0;
}

int unit_adler(int argc, char* argv[])
{
    (void)argc;
    (void*)argv;

    RETURN_ON_ERROR(test_init());
    RETURN_ON_ERROR(test_get());
    RETURN_ON_ERROR(test_adler_single_call());
    RETURN_ON_ERROR(test_adler_multi_call());
    RETURN_ON_ERROR(test_adler_1M_buffer());
    return 0;
}
