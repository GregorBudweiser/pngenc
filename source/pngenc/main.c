#include "pngenc.h"
#include "utils.h"
#include <string.h>
#include <malloc.h>

int main() {
    const uint32_t W = 1920;
    const uint32_t H = 1080;
    const uint32_t C = 3;

    uint8_t * buf = malloc(W*H*C);
    memset(buf, 0x0, W*H*C);

    uint32_t i;
    for(i = 0; i < W*H*C; i++)
        buf[i] = i;

    pngenc_image_desc desc;
    desc.data = buf;
    desc.width = W;
    desc.height = H;
    desc.num_channels = C;
    desc.row_stride = (uint64_t)W*(uint64_t)C;
    desc.strategy = PNGENC_NO_COMPRESSION;
    desc.bit_depth = 8;
    RETURN_ON_ERROR(pngenc_write_file(&desc, "img_uncompressed.png"));

    desc.strategy = PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1;
    RETURN_ON_ERROR(pngenc_write_file(&desc, "img_compressed.png"));

    //desc.strategy = PNGENC_FULL_COMPRESSION;
    //RETURN_ON_ERROR(pngenc_write_file(&desc, "img_full_compressed.png"));

    free(buf);

    return 0;
}
