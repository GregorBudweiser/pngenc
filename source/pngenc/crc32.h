#pragma once
#include <stdlib.h>
#include <stdint.h>

uint32_t crc32c(uint32_t crc, const void *buf, size_t len);
uint32_t copy_on_crc32c(uint32_t crci, const void *buf,
                        size_t len, uint8_t * dst);
