#include "huffman.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

// Enable/disable more timing info
#if 0
#include "../../tests/common.h"
#else
#define TIMING_START
#define TIMING_END
#define TIMING_END_MB(x)
#endif

static const uint8_t bit_reverse_table_256[] =
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

typedef struct _symbol {
    uint32_t probability; // TODO: Assert probabilities < 32bit => sum of elements < 4GB
    int16_t symbol;      // Index
    int16_t parent;      // Parent index
} Symbol;

void swap_symbols(Symbol * a, Symbol * b) {
    Symbol tmp = *a;
    *a = *b;
    *b = tmp;
}

int compare_by_probability(const void* a, const void* b) {
    return (int)(((const Symbol*)a)->probability
               - ((const Symbol*)b)->probability);
}

int compare_by_symbol(const void* a, const void* b) {
    return ((const Symbol*)a)->symbol
         - ((const Symbol*)b)->symbol;
}

void huffman_encoder_init(huffman_encoder * encoder) {
    memset(encoder, 0, sizeof(huffman_encoder));
}

void huffman_encoder_add(uint32_t * histogram, const uint8_t * symbols,
                         uint32_t length) {
    if(sizeof(size_t) == 8) {
        huffman_encoder_add64(histogram, symbols, length);
    } else {
        huffman_encoder_add32(histogram, symbols, length);
    }
}

/**
 * Same as huffman_encoder_add_simple just optimized for 64bit machines.
 */
void huffman_encoder_add64(uint32_t * histogram, const uint8_t * symbols,
                           uint32_t length) {
    // padd to 8 bytes
    while(length > 0 && (((size_t)symbols) & 0x7) != 0) {
        histogram[*symbols]++;
        symbols++;
    }

    uint16_t counters[4][256];
    uint64_t l8 = length/8;

    const uint64_t * data = (const uint64_t*)symbols;

    // Each sub-histogram gets updated twice -> 2*0x7FFF = UINT16_MAX
    for(uint64_t start = 0; start < length; start += 0x7FFF) {
        memset(counters, 0, 4*256*2);
        uint64_t end = min_u64(l8, start + 0x7FFF);
        for(uint64_t i = start; i < end; i++) {
            register uint64_t tmp = data[i];
            counters[0][tmp & 0xFF]++;
            counters[1][(tmp >>  8) & 0xFF]++;
            counters[2][(tmp >> 16) & 0xFF]++;
            counters[3][(tmp >> 24) & 0xFF]++;
            counters[0][(tmp >> 32) & 0xFF]++;
            counters[1][(tmp >> 40) & 0xFF]++;
            counters[2][(tmp >> 48) & 0xFF]++;
            counters[3][(tmp >> 56)]++;
        }

        for(int i = 0; i < 256; i++) {
            histogram[i] += counters[0][i] + counters[1][i]
                          + counters[2][i] + counters[3][i];
        }
    }

    // Unpadded trailing bytes..
    for(uint64_t i = l8*8; i < length; i++) {
        histogram[symbols[i]]++;
    }
}


void huffman_encoder_add32(uint32_t * histogram, const uint8_t * symbols,
                           uint32_t length) {
    // padd to 8 bytes
    while(length > 0 && (((size_t)symbols) & 0x7) != 0) {
        histogram[*symbols]++;
        symbols++;
        length--;
    }

    uint16_t counters[4][256];
    memset(counters, 0, 4*256*2);
    uint32_t l8 = length/4;

    const uint32_t * data = (const uint32_t*)symbols;

    // Each sub-histogram gets updated twice -> 2*0x7FFF = UINT16_MAX
    for(uint32_t start = 0; start < length; start += 0xFFFF) {
        uint32_t end = min_u32(l8, start + 0xFFFF);
        for(uint32_t i = start; i < end; i++) {
            register uint32_t tmp = data[i];
            counters[0][tmp & 0xFF]++;
            counters[1][(tmp >>  8) & 0xFF]++;
            counters[2][(tmp >> 16) & 0xFF]++;
            counters[3][tmp >> 24]++;
        }

        for(int i = 0; i < 256; i++) {
            histogram[i] += counters[0][i] + counters[1][i]
                          + counters[2][i] + counters[3][i];
        }
        memset(counters, 0, 4*256*2);
    }

    // Unpadded trailing bytes..
    for(uint64_t i = l8*4; i < length; i++) {
        histogram[symbols[i]]++;
    }
}

void huffman_encoder_add_simple(uint32_t * histogram, const uint8_t * data,
                                uint32_t length) {
    uint32_t i = 0;
    for(; i < length; i++) {
        histogram[data[i]]++;
    }
}

/**
 * Note: By definition of RLE distance is only ever 1.
 */
void huffman_encoder_add_rle_simple(uint32_t *histogram, const uint8_t * src,
                                    uint32_t num_bytes) {
    const uint32_t MAX_LEN = 258;
    uint32_t i = 0;
    while(i < num_bytes) {
        const uint8_t byte = src[i];
        histogram[byte]++;
        i++;

        uint32_t length = 0;
        while(i < num_bytes && src[i] == byte && length < MAX_LEN) {
            length++;
            i++;
        }

        if(length) {
            if (length < 3) {
                histogram[byte] += length;
            } else if (length == 258) {
                histogram[285]++;
            } else {
                // length code
                length -= 3;
                int32_t num_extra_bits = highest_bit_set(length);
                num_extra_bits = max_i32(num_extra_bits-2, 0);
                uint32_t length_symbol = 257 + (num_extra_bits << 2) + (length >> num_extra_bits);
                histogram[length_symbol]++;
            }
        }
    }
}

void huffman_encoder_add_rle(uint32_t *histogram, const uint8_t * src,
                             uint32_t num_bytes) {
    TIMING_START;
    const uint32_t MAX_LEN = 258;
    uint32_t lengths[259];
    memset(lengths, 0, sizeof(lengths));
    uint32_t i = 0;
    while(i < num_bytes) {
        // collect literal
        const uint8_t byte = src[i];
        histogram[byte]++;
        i++;

        // compute length
        uint32_t length = 0;
        while(i < num_bytes && src[i] == byte && length < MAX_LEN) {
            length++;
            i++;
        }

        if (length < 3) {
            histogram[byte] += length;
        } else {
            lengths[length]++;
        }
    }

    for(uint32_t length = 3; length < MAX_LEN; length++) {
        // length code
        uint32_t l = length - 3;
        int32_t num_extra_bits = highest_bit_set(l);
        num_extra_bits = max_i32(num_extra_bits-2, 0);
        uint32_t length_symbol = 257 + (num_extra_bits << 2) + (l >> num_extra_bits);
        histogram[length_symbol] += lengths[length];
    }
    histogram[285] = lengths[MAX_LEN];
    TIMING_END;
}

typedef struct _sp {
    uint16_t index;
    int32_t probability;
} sp;

int sort_sp_by_p(void const* a, void const* b) {
    return ((sp const*)a)->probability - ((sp const*)b)->probability;
}

/**
 * @param power: Higher values give better codes but need more iterations
 */
void huffman_encoder_build_tree_limited(huffman_encoder * encoder, uint8_t limit) {
    // build without limit first
    huffman_encoder_build_tree(encoder);
    if(huffman_encoder_get_max_length(encoder) <= limit) {
        return;
    }

    // code length counts
    int16_t counts[32];
    memset(counts, 0, sizeof(counts));
    for(uint32_t i = 0; i < HUFF_MAX_SIZE; i++) {
        counts[encoder->code_lengths[i]]++;
    }

    int16_t overflow = 0;
    for(uint32_t i = 31; i > limit; i--) {
        overflow += counts[i];
        counts[i-1] += counts[i] >> 1;
    }

    do {
        uint8_t bits = limit - 1;
        while(counts[bits] == 0) {
            bits--;
            assert(bits > 0);
        }
        counts[bits]--;
        counts[bits + 1] += 2;
        overflow -= 2;
    } while(overflow > 0);

    // sort symbols by probability and keep symbol index to update later
    sp list[HUFF_MAX_SIZE];
    for(int i = 0; i < HUFF_MAX_SIZE; i++) {
        list[i].probability = encoder->histogram[i];
        list[i].index = i;
    }

    qsort(list, HUFF_MAX_SIZE, sizeof(sp), &sort_sp_by_p);
    uint32_t j = counts[0];
    for(int bits = limit; bits > 0; bits--) {
        for(uint16_t i = 0; i < counts[bits]; i++) {
            encoder->code_lengths[list[j].index] = bits;
            j++;
        }
    }
    assert(huffman_encoder_get_max_length(encoder) <= limit);
}

void huffman_encoder_build_tree(huffman_encoder * encoder) {

    //const uint64_t N_SYMBOLS = HUFF_MAX_SIZE;
    // msvc cant do this because it lacks c99 support?
    //Symbol symbols[2*N_SYMBOLS-1];
    Symbol symbols[2*HUFF_MAX_SIZE-1];
    Symbol * next_free_symbol = symbols;
    int16_t next_free_symbol_value = HUFF_MAX_SIZE;

    // TODO: remove
    memset(symbols, 0, sizeof(Symbol)*(2*HUFF_MAX_SIZE-1));

    // Push raw symbols
    uint32_t i = 0;
    for( ; i < HUFF_MAX_SIZE; i++) {
        next_free_symbol->probability = encoder->histogram[i];
        next_free_symbol->symbol = (int16_t)i;
        next_free_symbol->parent = -1;

        // ignore symbols that don't appear
        next_free_symbol += (int)(encoder->histogram[i] > 0);
    }

    // number of symbols with non-zero probability
    const uint16_t num_symbols = (uint16_t)(next_free_symbol - symbols);

    // sort by probability
    qsort(symbols, num_symbols, sizeof(Symbol), &compare_by_probability);


    /* ==============
     * * BUILD TREE *
     * ==============
     *
     *
     * Iteration 1:
     * ============
     *
     *                           Take two lowest.
     * symbols   begin_symbol      Cobine into     unassigned
     *   |       |                  new one.          |
     *   v       v                     v              v
     * [ 0 | 1 |~2~|~3~|~.....~|~next_free_symbol~| ..... ] <- sorted by prob (except for the new one)
     *   ^   ^ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨
     * Two loweset prob. symbols       ^
     *                              Symbol range to filter / current tree
     *
     *
     * Iteration 2:
     * ============
     *
     * symbols      begin_symbol                     unassigned
     *   |               |                                |
     *   v               v                                v
     * [ 0 | 1 | 2 | 3 |~4~|~.....~|~next_free_symbol~| ..... ]
     *           ^   ^ ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨
     * Two loweset prob. symbols       ^
     *                              Symbol range to filter / current tree
     *                              Shrinks at front in each iteration (-2).
     *                              Grows at end in each iteration (+1).
     */
    Symbol * begin_symbol = symbols;
    i = num_symbols;
    while(i > 1) {
        // Combine the two lowest symbols into a new one.

        // Pick new symbol at the back of the buffer
        next_free_symbol->probability = begin_symbol[0].probability
                                      + begin_symbol[1].probability;
        begin_symbol[0].parent = next_free_symbol_value;
        begin_symbol[1].parent = next_free_symbol_value;
        next_free_symbol->symbol = next_free_symbol_value;
        next_free_symbol->parent = -1;
        next_free_symbol++;
        next_free_symbol_value++;

        // Skip the two symbols that just got combined
        begin_symbol++;
        begin_symbol++;

        i--;
        Symbol * curr = next_free_symbol - 1;
        while(curr > begin_symbol
              && curr->probability < (curr-1)->probability) {
            swap_symbols(curr, curr-1);
            curr--;
        }
    }

    // Get symbol lengths
    qsort(symbols, (size_t)(next_free_symbol - symbols),
          sizeof(Symbol), &compare_by_symbol);

    // compute symbol table first
    int16_t symbol_table[2*HUFF_MAX_SIZE-1];
    memset(symbol_table, 0, sizeof(int16_t)*(2*HUFF_MAX_SIZE-1));
    uint32_t count = (uint32_t)(next_free_symbol - symbols);
    assert(count <= 2*HUFF_MAX_SIZE-1);
    for(i = 0; i < count; i++) {
        symbol_table[symbols[i].symbol] = (int16_t)i;
    }

    // Get actual lengths
    assert(num_symbols <= HUFF_MAX_SIZE);
    for(i = 0; i < num_symbols; i++) {
        uint8_t length = 0;
        Symbol * current_symbol = symbols + i;
        while(current_symbol->parent >= 0) {
            current_symbol = symbols + symbol_table[current_symbol->parent];
            length++;
        }

        encoder->code_lengths[symbols[i].symbol] = length;
    }
}

void huffman_encoder_build_codes_from_lengths(huffman_encoder * encoder) {
    // huffman codes are limited to 15bits
    const uint8_t MAX_BITS = 16;
    assert(huffman_encoder_get_max_length(encoder) <= MAX_BITS);

    // Find number of elements per code length
    uint8_t num_code_lengths[16]; // for msvc... [MAX_BITS];
    memset(num_code_lengths, 0, sizeof(uint8_t)*MAX_BITS);

    uint32_t i;
    for(i = 0; i < HUFF_MAX_SIZE; i++) {
        if(encoder->code_lengths[i] > 0)
            num_code_lengths[encoder->code_lengths[i]-1]++;
    }

    // Generate base code for each length (see deflate rfc 1951)
    uint32_t next_code[16];
    memset(next_code, 0, sizeof(uint32_t)*MAX_BITS);
    uint32_t code = 0;
    uint32_t bits;
    for(bits = 1; bits < MAX_BITS; bits++) {
        code = (code + num_code_lengths[bits-1]) << 1;
        next_code[bits] = code;
    }

    // Generate final codes
    for (i = 0;  i < HUFF_MAX_SIZE; i++) {
        uint8_t len = encoder->code_lengths[i];
        if (len > 0) {
            encoder->symbols[i] = (uint16_t)next_code[len-1];
            next_code[len-1]++;
        }
    }

    // bitflip codes:
    for (i = 0; i < HUFF_MAX_SIZE; i++) {
        uint32_t symbol = (uint32_t)encoder->symbols[i] << (16-encoder->code_lengths[i]);
        symbol = ((uint32_t)bit_reverse_table_256[symbol & 0xFF] << 8)
               | ((uint32_t)bit_reverse_table_256[(symbol & 0xFF00) >> 8]);
        encoder->symbols[i] = (uint16_t)symbol;
    }
}

void huffman_encoder_encode(const huffman_encoder * encoder, const uint8_t * src,
                            uint32_t length, uint8_t * dst, uint64_t * offset) {
#if PNGENC_X86
    huffman_encoder_encode64_4(encoder, src, length, dst, offset);
#else
    huffman_encoder_encode64_3(encoder, src, length, dst, offset);
#endif
}

void huffman_encoder_encode64_4(const huffman_encoder * encoder, const uint8_t * src,
                                uint32_t length, uint8_t * dst, uint64_t * offset) {
    TIMING_START;
    const uint32_t padding = 64; // 64 bit / min Symbol size
    if(length <= padding) {
        huffman_encoder_encode_simple(encoder, src, length, dst, offset);
        return;
    }

    const uint16_t * sym = encoder->symbols;
    const uint8_t * nbits = encoder->code_lengths;
    const size_t fastEnd = length - padding;

    uint64_t bit_offset = *offset;
    uint8_t * dst2 = dst+(bit_offset>>3);
    uint64_t window = *dst2;
    uint64_t* window_ptr = (uint64_t*)dst2;

    size_t i;
    for(i = 0; i < fastEnd; i+=3) {
        uint64_t local_bit_offset = bit_offset & 7;

        // start with new local window to allow either unrolling or out-of-order processing
        uint64_t accum_symbols = (uint64_t)(sym[src[i]]);
        uint64_t accum_num_bits = nbits[src[i]];

        accum_symbols |= (uint64_t)(sym[src[i+1]]) << accum_num_bits;
        accum_num_bits += nbits[src[i+1]];

        accum_symbols |= (uint64_t)(sym[src[i+2]]) << accum_num_bits;
        accum_num_bits += nbits[src[i+2]];

        // Combine
        window |= accum_symbols << local_bit_offset;
        local_bit_offset += accum_num_bits;
        bit_offset = (bit_offset & ~7) + local_bit_offset;
        *window_ptr = window;

        // prepare next iteration
        uint8_t * dst3 = dst+(bit_offset>>3);
        window_ptr = (uint64_t*)dst3;
        size_t bytes_moved = dst3 - dst2;
        window = window >> (8*bytes_moved);
        dst2 = dst3;
    }

    // Padding
    for(; i < length; i++) {
        push_bits(encoder->symbols[src[i]], encoder->code_lengths[src[i]], dst, &bit_offset);
    }

    *offset = bit_offset;
    TIMING_END;
}

/**
 * This function does not have unaligned memory writes like the other encode
 * functions. Puts less pressure on MMU and scales better when parallelized.
 */
void huffman_encoder_encode64_3(const huffman_encoder * encoder,
                                const uint8_t * src, uint32_t length,
                                uint8_t * dst, uint64_t * offset) {
    TIMING_START;
    const uint32_t padding = 32; // 64 bit / min Symbol size
    if(length <= padding) {
        huffman_encoder_encode_simple(encoder, src, length, dst, offset);
        return;
    }
    const uint64_t fastEnd = length - padding;
    uint8_t * start = (dst + (*offset >> 3)); // byte-offset from bit-offset
    start = (uint8_t*)((uintptr_t)start & ~0x3ULL); // 4-byte aligned
    uint32_t bit_offset = *offset & 31ul;      // 4-byte aligned bit-offset
    uint32_t* ptr = (uint32_t*)start;         // Pointer to the current window
    uint64_t window = *ptr;

    uint64_t i = 0;
    for(; i < fastEnd; ) {
        // Add symbols until we have 32bits to write out
        do { // do first iteration unconditionally
            window |= ((uint64_t)encoder->symbols[src[i]]) << bit_offset;
            bit_offset += encoder->code_lengths[src[i]];
            window |= ((uint64_t)encoder->symbols[src[i+1]]) << bit_offset;
            bit_offset += encoder->code_lengths[src[i+1]];
            i += 2;
        } while(bit_offset < 32);

        *ptr = (uint32_t)window; // Write out compressed stuff in chunks of 32bits
        window = window >> 32;
        ptr++;
        bit_offset &= 31;
    }

    *ptr = (uint32_t)window; // Write out remaining bits
    *(ptr+1) = 0UL; // Clear next 64bits to allow subsequent access via 64bit reads
    *(ptr+2) = 0UL;
    bit_offset += (uint64_t)(((uint8_t*)ptr) - dst) * 8ULL;
    *offset = bit_offset;

    // Padding
    for(; i < length; i++) {
        uint8_t current_byte = src[i];
        push_bits(encoder->symbols[current_byte],
                  encoder->code_lengths[current_byte], dst, offset);
    }

    dst[(*offset >> 3)+1] = 0;
    dst[(*offset >> 3)+2] = 0;
    dst[(*offset >> 3)+3] = 0;
    TIMING_END;
}

/**
 * Simple reference implementation.
 */
void huffman_encoder_encode_simple(const huffman_encoder * encoder,
                                   const uint8_t * src, uint32_t length,
                                   uint8_t * dst, uint64_t * offset) {
    size_t i = 0;
    for(; i < length; i++) {
        uint8_t current_byte = src[i];
        push_bits(encoder->symbols[current_byte],
                  encoder->code_lengths[current_byte], dst, offset);
    }
}

/**
 * Simple reference implementation.
 */
void huffman_encoder_encode_rle_simple(const huffman_encoder * encoder,
                                       const huffman_encoder * dist_encoder,
                                       const uint8_t * src, uint32_t num_bytes,
                                       uint8_t * dst, uint64_t * bit_offset) {
    const uint32_t MAX_LEN = 257;
    uint32_t i = 0;
    while(i < num_bytes) {
        const uint8_t byte = src[i];
        push_bits(encoder->symbols[byte], encoder->code_lengths[byte], dst, bit_offset);
        i++;

        uint32_t length = 0;
        while(i < num_bytes && src[i] == byte && length < MAX_LEN) {
            length++;
            i++;
        }

        if(length) {
            if (length < 3) {
                for(int j = 0; j < length; j++) {
                    push_bits(encoder->symbols[byte], encoder->code_lengths[byte],
                              dst, bit_offset);
                }
            } else {
                // length code
                length -= 3;
                int32_t num_extra_bits = highest_bit_set(length);
                num_extra_bits = max_i32(num_extra_bits-2, 0);
                uint32_t length_symbol = 257 + (num_extra_bits << 2) + (length >> num_extra_bits);
                push_bits(encoder->symbols[length_symbol], encoder->code_lengths[length_symbol],
                          dst, bit_offset);

                // length extra bits
                uint32_t mask = (0x1 << num_extra_bits) - 1;
                uint32_t extra_bits = length & mask;
                push_bits(extra_bits, num_extra_bits, dst, bit_offset);

                // backward distance = 1 (definition of RLE); => dist symbol 0
                push_bits(dist_encoder->symbols[0], dist_encoder->code_lengths[0],
                          dst, bit_offset);
            }
        }
    }
}

/**
 * Length code + extra bits + backward dist zero bit
 */
struct length_extra {
    uint32_t bits;
    uint8_t nbits;
};

void huffman_encoder_encode_rle(const huffman_encoder * encoder,
                                const huffman_encoder * dist_encoder,
                                const uint8_t * src, uint32_t num_bytes,
                                uint8_t * dst, uint64_t * bit_offset) {
    TIMING_START;
    // Zero next 4 bytes to avoid masking step in push_bits_a()
    uint64_t tmp = *bit_offset;
    push_bits(0, 32, dst, &tmp);

    // build combined length + extra bit symbols in advance
    const uint32_t MAX_LEN = 258;
    struct length_extra tbl[259]; // [MAX_LEN+1]
    for(uint32_t length = 3; length < MAX_LEN; length++) {
        // length code
        uint32_t l = length - 3;
        //printf("0x%08x\n", l);
        int32_t num_extra_bits = highest_bit_set(l);
        num_extra_bits = max_i32(num_extra_bits-2, 0);
        uint32_t length_symbol = 257 + (num_extra_bits << 2) + (l >> (uint16_t)num_extra_bits);

        // length extra bits
        uint32_t mask = (0x1 << num_extra_bits) - 1;
        uint32_t extra_bits = l & mask;

        uint32_t bits = encoder->symbols[length_symbol];
        uint8_t n_bits = encoder->code_lengths[length_symbol];

        bits |= extra_bits << n_bits;
        n_bits += num_extra_bits+1; // add distance zero-bit too

        tbl[length].bits = bits;
        tbl[length].nbits = n_bits;
    }
    tbl[258].bits = encoder->symbols[285];
    tbl[258].nbits = encoder->code_lengths[285]+1;

    uint64_t offset = *bit_offset;
    uint64_t global_window = dst[offset >> 3];

    uint32_t i = 0;
    while(i < num_bytes) {
        const uint8_t byte = src[i];
        uint64_t window = encoder->symbols[byte];
        uint32_t n_bits = encoder->code_lengths[byte];
        i++;

        uint32_t length = 0;
        while(i < num_bytes && src[i] == byte && length < MAX_LEN) {
            length++;
            i++;
        }

        if(length) {
            if (length < 3) {
                for(int j = 0; j < length; j++) {
                    window |= ((uint64_t)encoder->symbols[byte]) << n_bits;
                    n_bits += encoder->code_lengths[byte];
                }
            } else {
                window |= ((uint64_t)(tbl[length].bits)) << n_bits;
                n_bits += tbl[length].nbits;
            }
        }

        global_window |= window << (offset & 7);
#if PNGENC_X86
        *((uint64_t*)(dst + (offset >> 3))) = global_window;
#else
        {
            uint8_t * dst2 = dst + (offset >> 3);
            uint64_t tmp_window = global_window;
            for (int i = 0; i < 8; i++) {
                *dst2++ = tmp_window & 0xFF;
                tmp_window >>= 8;
            }
        }
#endif
        size_t bytes_moved = ((offset + n_bits) >> 3) - (offset >> 3);
        global_window >>= bytes_moved << 3;
        offset += n_bits;
    }
    *bit_offset = offset;
    TIMING_END;
}

uint32_t huffman_encoder_get_max_length(const huffman_encoder * encoder) {
    uint32_t max_value = 0;
    for(int i = 0; i < HUFF_MAX_SIZE; i++) {
        max_value = max_u32(max_value, encoder->code_lengths[i]);
    }
    return max_value;
}

uint32_t huffman_encoder_get_num_literals(const huffman_encoder * encoder) {
    for(int i = HUFF_MAX_SIZE-1; i >= 0; i--) {
        if(encoder->histogram[i] > 0) {
            return (uint32_t)i;
        }
    }
    return 0;
}

void huffman_encoder_print(const huffman_encoder * encoder, const char * name) {
    printf("Huffman code for %s\n", name);
    for(int i = 0; i < HUFF_MAX_SIZE; i++) {
        if(encoder->code_lengths[i]) {
            printf(" > Code %d: %d bits (%d)\n", i,
                   encoder->code_lengths[i], encoder->symbols[i]);
        }
    }
}


/**
 * Push bits into a buffer at specified offset in bits.
 * Pre-condition: Requires the current byte to be zeroed after bit_offset.
 * Post-condition: Current byte is zeroed after bit_offset.
 *
 * Current byte:
 * <---Data--------><---Zeroed----->
 * _________________________________
 * | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
 * ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨
 * bit_offet % 8 ----^
 *
 */
void push_bits(uint64_t bits, uint8_t nbits, uint8_t * dst,
               uint64_t * bit_offset) {
    assert(nbits <= 64);

    // fill current byte
    uint64_t offset = *bit_offset;
    dst += offset >> 3;
    uint8_t shift = (uint8_t)(offset) & 0x7;
    *bit_offset = offset + nbits;
    int8_t bits_remaining = (int8_t)nbits;
    *dst = (*dst) | (uint8_t)(bits << shift);
    bits >>= 8 - shift;
    bits_remaining -= 8 - shift;

    // write remaining bytes (or clear next byte in case of -1)
    while(bits_remaining > -1) {
        dst++;
        *dst = (uint8_t)bits;
        bits >>= 8;
        bits_remaining -= 8;
    }
}
