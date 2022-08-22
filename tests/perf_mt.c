#include "common.h"
#include "../source/pngenc/pngenc.h"
#include "../source/pngenc/utils.h"
#include "../source/pngenc/splitter.h"
#include <string.h>
#include <malloc.h>

/**
 * Rather than writing to /dev/null (or nul on windows) we use an empty callback
 * because MSVC is kinda slow when using nul.
 */
int null_callback(const void * data, uint32_t data_len, void * user_data) {
    UNUSED(data);
    UNUSED(data_len);
    UNUSED(user_data);
    return PNGENC_SUCCESS;
}

int perf_mt(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

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
    desc.strategy = PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1;
    desc.bit_depth = 8;

    pngenc_encoder encoder = pngenc_create_encoder_default();
    for(int i = 0; i < 20; i++) {
        TIMING_START;
        pngenc_encode(encoder, &desc, null_callback, NULL);
        TIMING_END;
    }
    pngenc_destroy_encoder(encoder);

    free(buf);
    return 0;
}

