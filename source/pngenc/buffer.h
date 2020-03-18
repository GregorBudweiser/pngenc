#pragma once
#include <stdint.h>

typedef struct _buffer {
    uint8_t * data;
    uint32_t size;
    uint32_t pos;
} buffer;

int buffer_can_append(const buffer * buf, uint32_t num_bytes);
void buffer_append(buffer * buf, const uint8_t * src, uint32_t num_bytes);
void buffer_set_pos(buffer * buf, uint32_t pos);
