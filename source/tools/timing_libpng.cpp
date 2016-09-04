#include <png.h>
#include <cstdio>
#include <cstdint>
#include "../system/TimerLog.h"
//#include "../fastpng/imageData.h"
#include <cstring>

int main()
{
    const int W = 1920;
    const int H = 1080;

    uint8_t *mydata = new uint8_t[3*W*H];
    {
        FILE * file;
        file = fopen("data.bin", "rb");
        fread(mydata, 3, W*H, file);
        fclose(file);
    }

    FILE * fp;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    png_byte ** row_pointers = NULL;
    /* "status" contains the return value of this function. At first
           it is set to a value which means 'failure'. When the routine
           has finished its work, it is set to a value which means
           'success'. */
    int status = -1;
    /* The following number is set by trial and error only. I cannot
           see where it it is documented in the libpng manual.
        */
    int pixel_size = 3;
    int depth = 8;

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        goto png_create_write_struct_failed;
    }

    /*png_set_compression_level(png_ptr, 2);
    png_set_compression_strategy(png_ptr, 2);
    png_set_filter(png_ptr, 0, PNG_FILTER_SUB);*/

    png_set_compression_level(png_ptr, 0);
    png_set_compression_strategy(png_ptr, 0);
    png_set_filter(png_ptr, 0, PNG_FILTER_NONE);

    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        goto png_create_info_struct_failed;
    }

    /* Set up error handling. */

    if (setjmp (png_jmpbuf (png_ptr))) {
        goto png_failure;
    }

    /* Set image attributes. */

    png_set_IHDR (png_ptr,
                  info_ptr,
                  W,
                  H,
                  depth,
                  PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);

    /* Initialize rows of PNG. */


    row_pointers = (png_byte **)png_malloc (png_ptr, H * sizeof (png_byte *));
    for (y = 0; y < H; ++y) {
        //png_byte *row = (png_byte*)png_malloc (png_ptr, sizeof (uint8_t) * W * pixel_size);
        row_pointers[y] = (png_byte*)mydata+y*W*pixel_size;
    }

    /* Write the image data to "fp". */


    {
        LOG_TIME();
        char name[1000];
        for(int i = 0; i < 10; i++)
        {
            LOG_TIME();
            sprintf (name, "out%04d.png", i);
            fp = fopen (name, "wb");
            png_init_io (png_ptr, fp);
            {
                png_set_rows (png_ptr, info_ptr, row_pointers);
                png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
            }
            fclose(fp);
        }

        for(int i = 0; i < 10; i++)
        {
            LOG_TIME();
#if defined(WIN32) || defined(__WIN32)
            fp = fopen ("NUL", "wb");
#else
            fp = fopen ("/dev/null", "wb");
#endif
            png_init_io (png_ptr, fp);
            {
                png_set_rows (png_ptr, info_ptr, row_pointers);
                png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
            }
            fclose(fp);
        }
    }

    /* The routine has successfully written the file, so we set
           "status" to a value which indicates success. */

    status = 0;

    for (y = 0; y < H; y++) {
        //png_free (png_ptr, row_pointers[y]);
    }
    png_free (png_ptr, row_pointers);

png_failure:
png_create_info_struct_failed:
    png_destroy_write_struct (&png_ptr, &info_ptr);
png_create_write_struct_failed:
    //fclose (fp);
fopen_failed:
    return status;
}
