#include "../pngenc/pngenc.h"
#include "../../tests/common.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>

int main() {
    const int W = 1920;
    const int H = 1080;
    const int C = 3;

    // Load raw image data (1920x1080x3 bytes)
    FILE * file = fopen("data.bin", "rb");
    if(!file) {
        printf("Could not find raw image data input.\n");
        printf("Please provide binary image data in the file 'data.bin'.\n");
        printf("Expecting 1920x1080x3 bytes of data.\n");
        return 0;
    }

    uint8_t * buf = (uint8_t*)malloc(W*H*C);
    fread(buf, C, W*H, file);
    fclose(file);

    pngenc_image_desc desc;
    desc.data = buf;
    desc.width = W;
    desc.height = H;
    desc.num_channels = C;
    desc.row_stride = W*C;
    desc.bit_depth = 8;

    pngenc_encoder enc = pngenc_create_encoder(0, 1*1024*1024); // pngenc_create_encoder_default(); //
    for(int mode = 0; mode < 3; mode++)
    {
        desc.strategy = mode;

        char* mode_str[3] = { "uncompressed", "huff_only", "rle" };
        char* format[3] = { "pngenc_u_%03d.png", "pngenc_huff_%03d.png", "pngenc_rle_%03d.png"};
        char name[1000];
        printf("Write to disk (mode = %s):\n", mode_str[mode]);
        for(int i = 0; i < 1; i++)
        {
            TIMING_START;
            sprintf(name, format[mode], i);
            RETURN_ON_ERROR(pngenc_write(enc, &desc, name));
            TIMING_END;
        }

        printf("Write to /dev/null (mode = %s):\n", mode_str[mode]);
        for(int i = 0; i < 5; i++)
        {
            TIMING_START;
#if defined(WIN32) || defined(__WIN32)
            RETURN_ON_ERROR(pngenc_write(enc, &desc, "NUL"));
#else
            RETURN_ON_ERROR(pngenc_write(enc, &desc, "/dev/null"));
#endif
            TIMING_END;
        }
    }

    pngenc_destroy_encoder(enc);
    free(buf);
    return 0;
}
