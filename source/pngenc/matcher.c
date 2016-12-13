#include "matcher.h"
#include "utils.h"
#include <string.h>

void update_entry(tbl_entry * entry, uint32_t position) {
    uint32_t i;
    for (i = 0; i < 8; i++) {
        if(entry->positions[i] == 0) {
            entry->positions[i] = position;
            entry->positions[(i+1) % 8] = 0;
            return;
        }
    }
}

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
    const uint32_t max_match_length = 32768 - 1;

    tbl_entry hash_table[1 << 12];
    memset(hash_table, 0, (1 << 12)*sizeof(tbl_entry));

    uint32_t i, j;
    for(i = 0; i < length-3; ) {
        uint32_t hash = hash_12b((uint32_t*)(buf + i));
        uint32_t best_pos = 0;
        uint32_t best_length = 0;

        // Try all entries for the current hash
        for(j = 0; j < 8; j++) {
            uint32_t proposed_pos = hash_table[hash].positions[j];
            uint32_t match_length = match_fwd(buf, min_u32(max_match_length,
                                                           i - proposed_pos),
                                              i, proposed_pos);

            // Pick best position
            if(match_length > best_length) {
                best_pos = proposed_pos;
                best_length = match_length;
            }
        }

        // Update stuff according to best entry
        update_entry(hash_table + hash, i);
        symbol_histogram[best_length]++;
        i += best_length + 1;
    }

    return 0;
}
