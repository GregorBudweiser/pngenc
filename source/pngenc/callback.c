#include "callback.h"
#include "pngenc.h"
#include "node_custom.h"
#include "utils.h"
#include <stdio.h>

int64_t user_write_wrapper(struct _pngenc_node * n, const uint8_t * data,
                           uint32_t size) {
    pngenc_node_custom * node = (pngenc_node_custom*)n;
    pngenc_user_write_callback user_provided_function = node->custom_data1;
    RETURN_ON_ERROR(user_provided_function(data, size, node->custom_data0));
    return size;
}


int write_to_file_callback(const void * data, uint32_t data_len,
                           void * user_data) {
    size_t bytesWritten = fwrite(data, 1, data_len, (FILE*)user_data);
    return bytesWritten == (size_t)data_len
            ? PNGENC_SUCCESS
            : PNGENC_ERROR_FILE_IO;
}
