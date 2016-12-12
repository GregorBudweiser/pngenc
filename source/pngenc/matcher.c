#include "matcher.h"
#include <string.h>

// 3 bytes to 12 bits hash function
uint32_t hash_12b(const uint32_t * data) {
    uint32_t current = *data;
    return ((current >> 12) ^ current) & 0x0FFF;
}

uint32_t match_fwd(const uint8_t * buf, uint32_t max_match_length,
                   uint32_t current_pos, uint32_t proposed_pos) {
    uint32_t i = 0;
    while(i < max_match_length && buf[proposed_pos+i] == buf[current_pos+i]) {
        i++;
    }
    return i;
}

uint32_t histogram(const uint8_t * buf, uint32_t length,
                   uint16_t * symbol_histogram) {
    // Deflate's maximum match
    const uint32_t max_match_length = 32768;

    uint32_t hash_table[1 << 12];
    memset(hash_table, (1 << 12)*4, 0);

    uint32_t i;
    for(i = 0; i < length-3; ) {
        uint32_t hash = hash_12b((uint32_t*)(buf + i));
        uint32_t match_length = match_fwd(buf, max_match_length,
                                          i, hash_table[hash]);
        symbol_histogram[match_length]++;
        hash_table[hash] = i;
        i += match_length + 1;
    }
}
