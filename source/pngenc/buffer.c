#include "buffer.h"


int buffer_can_append(const buffer * buf, uint32_t num_bytes) {
    // TODO: Does that cover all cases?
    return buf->pos < buf->size - num_bytes ? 1 : 0;
}
