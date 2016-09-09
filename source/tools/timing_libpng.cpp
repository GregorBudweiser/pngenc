/*
 * Adapted from
 * http://zarb.org/~gc/html/libpng.html
 */

#include <png.h>
#include <cstdio>
#include <cstdint>
#include "../system/TimerLog.h"
#include <cstring>

void write(const char * filename, bool compress,
           int width, int height, uint8_t * data) {

    uint8_t ** row_pointers = new uint8_t*[height];
    for(int y = 0; y < height; y++) {
        row_pointers[y] = data + 3*width*y;
    }

    FILE *fp = fopen(filename, "wb");
    if(!fp) abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();

    if (setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);


    if(compress) {
        png_set_compression_level(png, 2);
        png_set_compression_strategy(png, 2);
        png_set_filter(png, 0, PNG_FILTER_SUB);
    } else {
        png_set_compression_level(png, 0);
        png_set_compression_strategy(png, 0);
        png_set_filter(png, 0, PNG_FILTER_NONE);
    }

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
                png,
                info,
                width, height,
                8,
                PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT
                );
    png_write_info(png, info);

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    //png_set_filler(png, 0, PNG_FILLER_AFTER);

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    delete [] row_pointers;

    fclose(fp);
}

int main()
{
    const int W = 1920;
    const int H = 1080;
    const int C = 3;

    // Load raw image data (1920x1080x3 bytes)
    FILE * file = fopen("data.bin", "rb");
    if(!file) {
        printf("Could not find raw image data input.\n");
        printf("Please provide binary image data in the file 'data.bin'.\n");
        printf("Expecting 1920x1080x3 bytes of data.\n");
        return 0;
    }

    uint8_t *buf = new uint8_t[3*W*H];
    fread(buf, C, W*H, file);
    fclose(file);

    for(int j = 0; j < 2; j++)
    {
        bool compressed = (bool)j;

        LOG_TIME();
        char name[1000];
        for(int i = 0; i < 5; i++)
        {
            LOG_TIME();
            sprintf (name, "out%04d.png", i);
            write(name, compressed, W, H, buf);
        }

        for(int i = 0; i < 5; i++)
        {
            LOG_TIME();
#if defined(WIN32) || defined(__WIN32)
            write("nil", compressed, W, H, mydata);
#else
            write("/dev/null", compressed, W, H, buf);
#endif
        }
    }

    delete [] buf;
    return 0;
}


