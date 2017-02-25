#include "common.h"
#include <memory.h>
#include <malloc.h>
#include <pngenc/pngenc.h>

int integration_save_png16(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    const int W = 100;
    const int H = 100;
    const int C = 3;
    uint16_t * buf = (uint16_t*)malloc(C*W*H*2);
    memset(buf, 0, C*W*H*2);
    for(int y = 0; y < H; y++) {
        for(int x = 0; x < W; x++) {
            uint16_t val = (x*16 << 8);
            buf[(y*W+x)*C  ] = val;
            buf[(y*W+x)*C+1] = val;
            buf[(y*W+x)*C+2] = val;
        }
    }

    pngenc_image_desc desc;
    desc.data = (uint8_t*)buf;
    desc.width = W;
    desc.height = H;
    desc.num_channels = C;
    desc.row_stride = C*W*2;
    desc.bit_depth = 16;

    // Save uncompressed
    desc.strategy = PNGENC_NO_COMPRESSION;
    ASSERT_TRUE(pngenc_write_file(&desc, "i_save_png16_000.png") == PNGENC_SUCCESS);

    // Save compressed
    desc.strategy = PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1;
    ASSERT_TRUE(pngenc_write_file(&desc, "i_save_png16_001.png") == PNGENC_SUCCESS);
    free(buf);

    return 0;
}

