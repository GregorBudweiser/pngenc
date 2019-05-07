#pragma once
#include <stdint.h>

int write_to_file_callback(const void * data, uint32_t data_len,
                           void * user_data);
