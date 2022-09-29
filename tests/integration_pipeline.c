#include "common.h"
#include <pngenc/pngenc.h>
#include "pngenc/callback.h"
#include "string.h"
#include <omp.h>

int integration_pipeline(int argc, char* argv[]) {

    UNUSED(argc);
    UNUSED(argv);

    const int W = 1920;
    const int H = 1080;
    const int C = 3;

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
    desc.row_stride = W * C;
    desc.bit_depth = 8;
    desc.strategy = PNGENC_HUFF_ONLY;

    uint8_t * copies[8];
    copies[0] = buf;
    for (int i = 1; i < 8; i++) {
        copies[i] = (uint8_t*)malloc(W*H*C);
        memcpy(copies[i], buf, W*H*C);
    }


#if defined(WIN32) || defined(__WIN32)
    const char * devNull = "nul";
#else
    const char * devNull = "/dev/null";
#endif

    pngenc_encoder encoder = pngenc_create_encoder_default();
    for(int i = 0; i < 20; i++) {
        desc.data = copies[i % 8];
        TIMING_START;
        pngenc_write(encoder, &desc, devNull);
        TIMING_END;
    }
    pngenc_destroy_encoder(encoder);

    printf("---\n");

    for(int i = 0; i < 20; i++) {
        desc.data = copies[i % 8];
        TIMING_START;
        pngenc_write_file(&desc, devNull);
        TIMING_END;
    }

    free(buf);

    return PNGENC_SUCCESS;
}

