#include <stdint.h>

typedef struct _range_encoder {

} range_encoder;

uint32_t hash_12b(const uint32_t * data);

uint32_t match_fwd(const uint8_t * buf, uint32_t max_match_length,
                   uint32_t current_pos, uint32_t proposed_pos);

uint32_t histogram(const uint8_t * buf, uint32_t length,
                   uint16_t *symbol_histogram);
