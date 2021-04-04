/*
 * Adapted from
 * http://zarb.org/~gc/html/libpng.html
 */

#include <png.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../../tests/common.h"

void write(const char * filename, int width, int height, uint8_t * data,
           int level, int strategy, int filter) {

    uint8_t ** row_pointers = (uint8_t**)malloc(sizeof(uint8_t*)*height);
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

    png_set_compression_level(png, level);
    png_set_compression_strategy(png, strategy);
    png_set_filter(png, 0, filter);


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
    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    free(row_pointers);
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

    uint8_t *buf = (uint8_t*)malloc(3*W*H*sizeof(uint8_t));
    fread(buf, C, W*H, file);
    fclose(file);

    for(int compressed = 0; compressed < 2; compressed++)
    {
        printf("Write to disk (compressed = %d):\n", compressed);
        for(int i = 0; i < 10; i++)
        {
            char name[1000];
            TIMING_START;
            sprintf (name, "out%03d.png", i);
            if (compressed) {
                write(name, W, H, buf, 2, 2, PNG_FILTER_SUB);
            } else {
                write(name, W, H, buf, 0, 0, PNG_FILTER_NONE);
            }
            TIMING_END;
        }

        printf("Write to /dev/null (compressed = %d):\n", compressed);
        for(int i = 0; i < 10; i++)
        {
            TIMING_START;
#if defined(WIN32) || defined(__WIN32)
            const char * name = "nil";
#else
            const char * name = "/dev/null";
#endif
            if (compressed) {
                write(name, W, H, buf, 2, 2, PNG_FILTER_SUB);
            } else {
                write(name, W, H, buf, 0, 0, PNG_FILTER_NONE);
            }
            TIMING_END;
        }
    }

    free(buf);
    return 0;
}


