#include "java.h"
#include "pngenc.h"

jint Java_PngEnc_writePngFile(JNIEnv * env, jclass clz, jstring file_name,
                              jobject buffer, jint w, jint h, jint c,
                              jint strategy)
{
    uint8_t * data;
    data = (uint8_t*)(*env)->GetDirectBufferAddress(env, buffer);
    pngenc_image_desc desc;
    desc.data = data;
    desc.width = w;
    desc.height = h;
    desc.num_channels = c;
    desc.row_stride = (uint64_t)w*(uint64_t)c;
    desc.strategy = strategy;

    const char *native_string = (*env)->GetStringUTFChars(env, file_name, 0);
    int ret = write_png_file(&desc, native_string);
    //printf("%s\n", native_string);
    (*env)->ReleaseStringUTFChars(env, file_name, native_string);
    return ret;
}
