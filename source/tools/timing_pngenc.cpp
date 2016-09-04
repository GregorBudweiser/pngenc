extern "C" {
    #include "../pngenc/pngenc.h"
}
#include "../system/TimerLog.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <malloc.h>

int main() {
    const int W = 1920;
    const int H = 1080;
    const int C = 3;
    uint8_t * buf = (uint8_t*)malloc(W*H*C);

    {
        FILE * f = fopen("data.bin", "rb");
        fread(buf, C, W*H, f);
        fclose(f);
    }

    pngenc_image_desc desc;
    desc.data = buf;
    desc.width = W;
    desc.height = H;
    desc.num_channels = C;
    desc.row_stride = W*C;
    desc.strategy = PNGENC_NO_COMPRESSION;// PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1;


    int ret;
    {
        LOG_TIME();
        char name[1000];
        for(int i = 0; i < 10; i++)
        {
            LOG_TIME();
            sprintf(name, "out%03d.png", i);
            ret = write_png_file(&desc, name);
        }

        for(int i = 0; i < 10; i++)
        {
            LOG_TIME();
#if defined(WIN32) || defined(__WIN32)
            ret = write_png_file(&desc, "NUL");
#else
            ret = write_png_file(&desc, "/dev/null");
#endif
        }
    }
    free(buf);

    return ret;
}
