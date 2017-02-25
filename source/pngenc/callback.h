#pragma once
#include <stdint.h>
#include "node.h"

int64_t user_write_wrapper(struct _pngenc_node * n, const uint8_t * data,
                           uint32_t size);

int write_to_file_callback(const void * data, uint32_t data_len,
                           void * user_data);
