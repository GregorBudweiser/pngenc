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

    for(int compressed = 0; compressed < 2; compressed++)
    {
        desc.strategy = (compressed > 0)
                ? PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1
                : PNGENC_NO_COMPRESSION;


        char name[1000];
        printf("Write to disk (compressed = %d):\n", compressed);
        for(int i = 0; i < 10; i++)
        {
            TIMING_START;
            sprintf(name, "out%03d.png", i);
            RETURN_ON_ERROR(pngenc_write_file(&desc, name));
            TIMING_END;
        }

        printf("Write to /dev/null (compressed = %d):\n", compressed);
        for(int i = 0; i < 10; i++)
        {
            TIMING_START;
#if defined(WIN32) || defined(__WIN32)
            RETURN_ON_ERROR(pngenc_write_file(&desc, "NUL"));
#else
            RETURN_ON_ERROR(pngenc_write_file(&desc, "/dev/null"));
#endif
            TIMING_END;
        }
    }

    free(buf);
    return 0;
}
