#include "utils.h"


uint64_t min_u64(uint64_t a, uint64_t b) {
    return a < b ? a : b;
}

uint64_t min_i64(int64_t a, int64_t b) {
    return a < b ? a : b;
}

uint32_t min_u32(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

uint32_t max_u32(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

int32_t max_i32(int32_t a, int32_t b) {
    return a > b ? a : b;
}

uint32_t swap_endianness32(uint32_t value) {
    return ((value & 0x000000FF) << 24) | ((value & 0x0000FF00) << 8)
         | ((value & 0x00FF0000) >> 8 ) | ((value & 0xFF000000) >> 24);
}
