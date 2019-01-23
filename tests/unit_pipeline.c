#include <string.h>
#include "common.h"
#include "pngenc/pngenc.h"
#include "pngenc/callback.h"

int unit_pipeline(int argc, char* argv[]) {
    const int W = 400-1;
    const int H = 300;
    const int C = 1;

    uint8_t * buf = (uint8_t*)malloc(W*H*C);

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

    char filename[1024];
    FILE * file;
    memset(buf, 0, W*H*C);
    for(int i = 0; i < 4; i++) {
        for(int y = 0; y < H; y++) {
            for(int x = 0; x < W; x++) {
                uint8_t color = ((x > W/2) + 2*(y > H/2) == i) ? 0 : 255;
                for(int c = 0; c < C; c++) {
                    buf[(y*W+x)*C+c] = color;
                }
            }
        }

        memset(filename, 0, 1024);
        sprintf(filename, "unit_pipeline_%02d.png", i);
        file = fopen(filename, "wb");
        if (file == NULL) {
            printf("Could not open output file: %s\n", filename);
            return PNGENC_ERROR_FILE_IO;
        }

        // TODO: Automate comparison of output (vs non-pipeline?)
        RETURN_ON_ERROR(pngenc_pipeline_write(pipeline, &desc, file));
        //RETURN_ON_ERROR(pngenc_write_file(&desc, filename));

        if(fclose(file) != 0)
            return PNGENC_ERROR_FILE_IO;
    }

    pngenc_pipeline_destroy(pipeline);

    return PNGENC_SUCCESS;
}

