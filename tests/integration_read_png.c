#include "common.h"
#include <pngenc/pngenc.h>

int integration_read_png(int argc, char* argv[]) {
    UNUSED(argc)
    UNUSED(argv)

    TIMING_START

    pngenc_image_desc desc;
    int result = pngenc_read_file(&desc, "test.png");
    ASSERT_TRUE(result == PNGENC_SUCCESS)



    //RETURN_ON_ERROR(pngenc_write_file(descriptor, "out.png"))
    //ASSERT_TRUE(desc.width == 512);
    //ASSERT_TRUE(desc.height == 512);
    //ASSERT_TRUE(desc.num_channels == 3);
    //ASSERT_TRUE(desc.bit_depth == 8);
    TIMING_END
    return 0;
}

