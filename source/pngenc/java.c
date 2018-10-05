#include "java.h"
#include "pngenc.h"

jint Java_PngEnc_writePngFile(JNIEnv * env, jclass clz, jstring file_name,
                              jobject buffer, jint w, jint h, jint c,
                              jint bit_depth, jint strategy)
{
    pngenc_image_desc desc;
    desc.data = (uint8_t*)(*env)->GetDirectBufferAddress(env, buffer);
    desc.width = w;
    desc.height = h;
    desc.num_channels = c;
    desc.row_stride = (uint64_t)w*c*((uint8_t)bit_depth/8);
    desc.bit_depth = bit_depth;
    desc.strategy = strategy;

    const char *native_string = (*env)->GetStringUTFChars(env, file_name, 0);
    int ret = pngenc_write_file(&desc, native_string);

#if defined(_DEBUG) || defined(DEBUG)
    printf("FileName: %s\n", native_string);
    printf("W: %d, H: %d, C: %d\n", w, h, c);
    printf("Strategy: %d\n", strategy);
    printf("data: 0x%llx\n", desc.data);
#endif

    (*env)->ReleaseStringUTFChars(env, file_name, native_string);
    return ret;
}
