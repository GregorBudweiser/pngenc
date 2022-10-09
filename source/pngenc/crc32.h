#pragma once
#include <stdlib.h>
#include <stdint.h>

/**
 * Dispatches to suitable implementation based on platform.
 */
uint32_t crc32(uint32_t crc, const void *buf, size_t len);

uint32_t crc32_sw(uint32_t crci, const void *buf, size_t len);
void crc32_init_sw();
