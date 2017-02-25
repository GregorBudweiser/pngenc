#include "common.h"
#include <memory.h>
#include <malloc.h>
#include <pngenc/pngenc.h>

int integration_save_png(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    const int W = 100;
    const int H = 100;
    const int C = 3;
    uint8_t * buf = (uint8_t*)malloc(C*W*H);
    memset(buf, 0, C*W*H);
    for(int y = 0; y < H; y++) {
        for(int x = 0; x < W; x++) {
            uint8_t val = (uint8_t)(x << 4);
            buf[(y*W+x)*C  ] = val;
            buf[(y*W+x)*C+1] = val;
            buf[(y*W+x)*C+2] = val;
        }
    }

    pngenc_image_desc desc;
    desc.data = buf;
    desc.width = W;
    desc.height = H;
    desc.num_channels = C;
    desc.row_stride = C*W;
    desc.bit_depth = 8;

    // Save uncompressed
    desc.strategy = PNGENC_NO_COMPRESSION;
    ASSERT_TRUE(pngenc_write_file(&desc, "i_save_png_000.png") == PNGENC_SUCCESS);

    // Save compressed
    desc.strategy = PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1;
    ASSERT_TRUE(pngenc_write_file(&desc, "i_save_png_001.png") == PNGENC_SUCCESS);

    // Save fully compressed
    desc.strategy = PNGENC_FULL_COMPRESSION;
    ASSERT_TRUE(pngenc_write_file(&desc, "i_save_png_002.png") == PNGENC_SUCCESS);

    free(buf);

    return 0;
}

