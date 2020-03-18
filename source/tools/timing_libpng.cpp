/*
 * Adapted from
 * http://zarb.org/~gc/html/libpng.html
 */

#include <png.h>
#include <cstdio>
#include <cstdint>
#include "../system/TimerLog.h"
#include <cstring>

void write(const char * filename, int width, int height, uint8_t * data,
           int level, int strategy, int filter) {

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

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    //png_set_filler(png, 0, PNG_FILLER_AFTER);

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    delete [] row_pointers;

    fclose(fp);
}

#include <stdlib.h>
#include <stdio.h>
#include <png.h>

void read_png_file(const char *filename) {

  int width, height;
  png_byte color_type;
  png_byte bit_depth;
  png_bytep *row_pointers = NULL;

  FILE *fp = fopen(filename, "rb");

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png) abort();

  png_infop info = png_create_info_struct(png);
  if(!info) abort();

  if(setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  png_read_info(png, info);

  width      = png_get_image_width(png, info);
  height     = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth  = png_get_bit_depth(png, info);

  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt

  if(bit_depth == 16)
    png_set_strip_16(png);

  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);

  if(png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);

  // These color_type don't have an alpha channel then fill it with 0xff.
  if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

  if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

  png_read_update_info(png, info);

  if (row_pointers) abort();

  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }

  png_read_image(png, row_pointers);

  fclose(fp);

  png_destroy_read_struct(&png, &info, NULL);
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

        /*LOG_TIME();
        for(int i = 0; i < 10; i++)
        {
            char name[1000];
            LOG_TIME();
            sprintf (name, "out%03d.png", i);
            if (compressed) {
                read_png_file("out000.png");
                //write(name, W, H, buf, 2, 2, PNG_FILTER_SUB);
            } else {
                read_png_file("out000.png");
                //write(name, W, H, buf, 0, 0, PNG_FILTER_NONE);
            }
        }*/

        for(int i = 0; i < 10; i++)
        {
            LOG_TIME();
#if defined(WIN32) || defined(__WIN32)
            const char * name = "nil";
#else
            const char * name = "/dev/null";
#endif
            if (compressed) {
                read_png_file("out000.png");
                //write(name, W, H, buf, 2, 2, PNG_FILTER_SUB);
            } else {
                read_png_file("out000.png");
               // write(name, W, H, buf, 0, 0, PNG_FILTER_NONE);
            }
        }
    }

    delete [] buf;
    return 0;
}


