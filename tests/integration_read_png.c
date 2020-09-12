#include "common.h"
#include <pngenc/pngenc.h>

int integration_read_png(int argc, char* argv[]) {
    UNUSED(argc)
    UNUSED(argv)

    TIMING_START

    FILE * file;
    if((file =fopen("test.png", "rb")) == 0) {
        printf("Could not open input file!");
    }
    fclose(file);

    pngenc_image_desc desc;
    int result = pngenc_read_file(&desc, "test.png");

    TIMING_END

    ASSERT_TRUE(result == PNGENC_SUCCESS)
    ASSERT_TRUE(desc.width == 512);
    ASSERT_TRUE(desc.height == 512);
    ASSERT_TRUE(desc.num_channels == 3);
    ASSERT_TRUE(desc.bit_depth == 8);
    //RETURN_ON_ERROR(pngenc_write_file(descriptor, "out.png"))

    return 0;
}

