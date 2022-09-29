#include "common.h"
#include "string.h"

#include "../source/pngenc/deflate.h"

int test_deflate() {
    uint8_t src[512];
    uint8_t dst[1024];
    memset(src, 0, 512);

    int64_t r_compressed = write_deflate_block_huff_only(dst, src, 512, 1);
    int64_t r_uncompressed = write_deflate_block_uncompressed(dst, src, 512, 1);
    ASSERT_TRUE(r_compressed > 0);
    ASSERT_TRUE(r_uncompressed > 0);
    ASSERT_TRUE(r_compressed < r_uncompressed);

    return 0;
}

int unit_deflate(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    RETURN_ON_ERROR(test_deflate());

    return 0;
}

