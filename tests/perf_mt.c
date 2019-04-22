#include "common.h"
#include "../source/pngenc/pngenc.h"
#include "../source/pngenc/utils.h"
#include "../source/pngenc/splitter.h"
#include <string.h>
#include <malloc.h>

int perf_mt(int argc, char* argv[]) {
    const uint32_t W = 1920;
    const uint32_t H = 1080;
    const uint32_t C = 3;

    uint8_t * buf;
    {
        // Load raw image data (1920x1080x3 bytes)
        FILE * file = fopen("data.bin", "rb");
        if(!file) {
            printf("Could not find raw image data input.\n");
            printf("Please provide binary image data in the file 'data.bin'.\n");
            printf("Expecting 1920x1080x3 bytes of data.\n");
            return 0;
        }
        buf = (uint8_t*)malloc(W*H*C);
        fread(buf, C, W*H, file);
        fclose(file);
    }

    pngenc_image_desc desc;
    desc.data = buf;
    desc.width = W;
    desc.height = H;
    desc.num_channels = C;
    desc.row_stride = (uint64_t)W*(uint64_t)C;
    desc.strategy = PNGENC_NO_COMPRESSION;
    desc.bit_depth = 8;

    pngenc_encoder encoder = pngenc_create_encoder();

    pngenc_write(encoder, &desc, "C:\\Users\\RTFM2\\Desktop\\test.png");

    for(int i = 0; i < 10; i++) {
        TIMING_START;
        pngenc_write(encoder, &desc, "nul");
        TIMING_END;
    }

    free(buf);

    pngenc_destroy_encoder(encoder);

    return 0;
}

