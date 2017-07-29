#include <stdint.h>


#define NUM_HASH_ENTRIES 1

#define POT_HASH_TABLE_SIZE 12

/**
 * Hash table entry.
 * For each hash we are storing the most recent matches.
 */
typedef struct _tbl_entry {
    uint32_t positions[NUM_HASH_ENTRIES];
} tbl_entry;

/**
 * We only allow 12bit extra bits to store the bits + len(bits) in one short.
 * Thus we reduce the max window size to 16k!
 */
#define MAX_DIST_EXTRA_BITS 12


void update_entry(tbl_entry * entry, uint32_t position);

uint32_t hash_12b(const uint32_t * data);

uint32_t match_fwd(const uint8_t * buf, uint32_t max_match_length,
                   uint32_t current_pos, uint32_t proposed_pos);

uint32_t histogram(const uint8_t * buf, uint32_t length,
                   uint32_t *symbol_histogram, uint32_t * dist_histogram,
                   uint16_t * out_buf, uint32_t * out_length);

/**
 * Encode into temporary format.
 */
uint32_t encode_match_tmp(uint16_t * out, uint32_t out_i,
                          uint32_t match_length, uint32_t bwd_dist,
                          uint32_t * symbol_histogram,
                          uint32_t *dist_histogram);
