#include "common.h"
#include <memory.h>
#include <malloc.h>
#include <pngenc/pngenc.h>

static const int W = 160;
static const int H = 60;

static void fill(uint8_t * buf, int x0, int y0, int w, int h, uint8_t val, const int C) {
    for(int y = y0; y < y0+h; y++) {
        for(int x = x0; x < x0+w; x++) {
            for(int c = 0; c < C; c++) {
                if ((C == 2 || C == 4) && c+1 == C) { // is alpha channel?
                    buf[(y*W+x)*C+c] = 0xFF;
                } else {
                    buf[(y*W+x)*C+c] = val;
                }
            }
        }
    }
}

static int save(const uint8_t C) {
    uint8_t * buf = (uint8_t*)malloc(C*W*H);
    memset(buf, 0, C*W*H);

    fill(buf, 10, 10, 40, 40, 0x00, C);
    fill(buf, 60, 10, 40, 40, 0x88, C);
    fill(buf, 110, 10, 40, 40, 0xFF, C);

    pngenc_image_desc desc;
    desc.data = (uint8_t*)buf;
    desc.width = W;
    desc.height = H;
    desc.num_channels = C;
    desc.row_stride = C*W;
    desc.bit_depth = 8;

    char filename[256];

    // Save uncompressed
    desc.strategy = PNGENC_NO_COMPRESSION;
    sprintf(filename, "integration_save_png_%dC_uncomp.png", C);
    ASSERT_TRUE(pngenc_write_file(&desc, filename) == PNGENC_SUCCESS);

    // Save compressed (huffman only)
    desc.strategy = PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1;
    sprintf(filename, "integration_save_png_%dC_comp.png", C);
    ASSERT_TRUE(pngenc_write_file(&desc, filename) == PNGENC_SUCCESS);

    free(buf);

    return PNGENC_SUCCESS;
}

int integration_save_png(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    uint8_t c;
    for (c = 1; c <= 4; c++) {
        RETURN_ON_ERROR(save(c));
    }

    return PNGENC_SUCCESS;
}

