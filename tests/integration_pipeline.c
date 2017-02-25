#include "common.h"
#include <pngenc/pngenc.h>
#include "pngenc/callback.h"

int integration_pipeline(int argc, char* argv[]) {

    uint8_t buf[100];
    memset(buf, 0, 100);

    pngenc_image_desc image;
    image.bit_depth = 8;
    image.num_channels = 1;
    image.width = 10;
    image.height = 10;
    image.row_stride = 10;
    image.strategy = PNGENC_NO_COMPRESSION; //PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1;
    image.data = buf;

    pngenc_pipeline pipeline;
    pipeline = pngenc_pipeline_create(&image, &write_to_file_callback, NULL);

    FILE * file = fopen("blah.png", "wb");
    if(file == NULL)
        return PNGENC_ERROR_FILE_IO;

    TIMING_START;
    pngenc_pipeline_write(pipeline, &image, file);
    TIMING_END;

    if(fclose(file) != 0)
        return PNGENC_ERROR_FILE_IO;

    pngenc_pipeline_destroy(pipeline);

    return PNGENC_SUCCESS;
}

