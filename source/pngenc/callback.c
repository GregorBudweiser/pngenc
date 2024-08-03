#include "callback.h"
#include "pngenc.h"
#include <stdio.h>

int write_to_file_callback(const void * data, uint32_t data_len,
                           void * user_data) {
    size_t bytesWritten = fwrite(data, 1, data_len, (FILE*)user_data);
    return bytesWritten == (size_t)data_len
            ? PNGENC_SUCCESS
            : PNGENC_ERROR_FILE_IO;
}
