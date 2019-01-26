#include "common.h"
#include <pngenc/pngenc.h>
#include "pngenc/callback.h"

int integration_pipeline(int argc, char* argv[]) {

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
    printf("Could not open output file!\n");

    pngenc_image_desc desc;
    desc.data = buf;
    desc.width = W;
    desc.height = H;
    desc.num_channels = C;
    desc.row_stride = W * C;
    desc.bit_depth = 8;
    desc.strategy = PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1;

    pngenc_pipeline pipeline;
    pipeline = pngenc_pipeline_create(&desc, &write_to_file_callback, NULL);

#if defined(WIN32) || defined(__WIN32)
    const char * devNull = "nul";
#else
    const char * devNull = "/dev/null";
#endif

    FILE * file = fopen(devNull, "wb");
    if (file == NULL) {
        printf("Could not open output file!\n");
        return PNGENC_ERROR_FILE_IO;
    }

    for(int i = 0; i < 20; i++) {
        TIMING_START;
        pngenc_pipeline_write(pipeline, &desc, file);
        TIMING_END;
    }

    printf("---\n");

    for(int i = 0; i < 20; i++) {
        TIMING_START;
        pngenc_write_file(&desc, devNull);
        TIMING_END;
    }

    if(fclose(file) != 0)
        return PNGENC_ERROR_FILE_IO;

    pngenc_pipeline_destroy(pipeline);
    free(buf);

    return PNGENC_SUCCESS;
}

