#include "pngenc.h"
#include "utils.h"
#include <string.h>
#include <malloc.h>

int main() {
    const uint32_t W = 255;
    const uint32_t H = 255;
    const uint32_t C = 1;

    uint8_t * buf = malloc(W*H*C);
    memset(buf, 0x0, W*H*C);

    uint32_t i;
    for(i = 0; i < W*H*C; i++)
        buf[i] = (uint8_t)i;

    pngenc_image_desc desc;
    desc.data = buf;
    desc.width = W;
    desc.height = H;
    desc.num_channels = C;
    desc.row_stride = (uint64_t)W*(uint64_t)C;
    desc.bit_depth = 8;

    // save uncompressed
    desc.strategy = PNGENC_NO_COMPRESSION;
    RETURN_ON_ERROR(pngenc_write_file(&desc, "img_uncompressed.png"));

    // save compressed
    desc.strategy = PNGENC_HUFF_ONLY;
    RETURN_ON_ERROR(pngenc_write_file(&desc, "img_compressed.png"));


    free(buf);

    return 0;
}
