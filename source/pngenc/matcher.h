#include <stdint.h>

/**
 * Hash table entry.
 * For each hash we are storing the 8 most recent matches.
 */
#define NUM_HASH_ENTRIES 4
typedef struct _tbl_entry {
    uint32_t positions[NUM_HASH_ENTRIES];
} tbl_entry;

void update_entry(tbl_entry * entry, uint32_t position);

uint32_t hash_12b(const uint32_t * data);

uint32_t match_fwd(const uint8_t * buf, uint32_t max_match_length,
                   uint32_t current_pos, uint32_t proposed_pos);

uint32_t histogram(const uint8_t * buf, uint32_t length,
                   uint16_t *symbol_histogram);
