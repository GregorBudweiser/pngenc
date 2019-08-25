#include "huffman.h"
#include "utils.h"
#include "pngenc.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

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
    memset(counters, 0, 4*256*2);
    uint64_t l8 = length/8;

    const uint64_t * data = (const uint64_t*)symbols;

    // Each sub-histogram gets updated twice -> 2*0x7FFF = UINT16_MAX
    for(uint64_t start = 0; start < length; start += 0x7FFF) {
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
        memset(counters, 0, 4*256*2);
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
 * @param power: Higher values give better codes but need more iterations
 */
int huffman_encoder_build_tree_limited(huffman_encoder * encoder,
                                       uint8_t limit, power_coefficient power) {
    // TODO: Compute optimal tree (e.g. use optimal algorithm)
    RETURN_ON_ERROR(huffman_encoder_build_tree(encoder));
    while(huffman_encoder_get_max_length(encoder) > limit) {
        // nonlinearly reduce weight of nodes to lower max tree depth
        switch(power) {
        case POW_80:
            for(int i = 0; i < HUFF_MAX_SIZE; i++) {
                encoder->histogram[i] = fast_pow80(encoder->histogram[i]);
            }
            break;
        case POW_95: // fallthrough
        default:
            for(int i = 0; i < HUFF_MAX_SIZE; i++) {
                encoder->histogram[i] = fast_pow95(encoder->histogram[i]);
            }
            break;
        }
        RETURN_ON_ERROR(huffman_encoder_build_tree(encoder));
    }
    return PNGENC_SUCCESS;
}

int huffman_encoder_build_tree(huffman_encoder * encoder) {

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

    return PNGENC_SUCCESS;
}

int huffman_encoder_build_codes_from_lengths(huffman_encoder * encoder) {
    // huffman codes are limited to 15bits
    if(huffman_encoder_get_max_length(encoder) > 15) {
        return PNGENC_ERROR;
    }

    // Find number of elements per code length
    const uint8_t MAX_BITS = 16;
    uint8_t num_code_lengths[16]; // for msvc... [MAX_BITS];
    memset(num_code_lengths, 0, sizeof(uint8_t)*MAX_BITS);

    uint32_t i;
    for(i = 0; i < HUFF_MAX_SIZE; i++) {
        assert(encoder->code_lengths[i]-1 < MAX_BITS);
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
    for (i = 0;  i <= HUFF_MAX_SIZE; i++) {
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

    return PNGENC_SUCCESS;
}

int huffman_encoder_encode(const huffman_encoder * encoder, const uint8_t * src,
                           uint32_t length, uint8_t * dst, uint64_t * offset) {
    // Cannot assume dst to be zeroed!
    return huffman_encoder_encode64_3(encoder, src, length, dst, offset);
}

/**
 * This function does not have unaligned memory writes like the other encode
 * functions. Puts less pressure on MMU and scales better when parallelized.
 */
int huffman_encoder_encode64_3(const huffman_encoder * encoder,
                               const uint8_t * src, uint32_t length,
                               uint8_t * dst, uint64_t * offset) {
    const uint32_t padding = 32; // 64 bit / min Symbol size
    if(length <= padding) {
        return huffman_encoder_encode_simple(encoder, src, length, dst, offset);
    }
    const uint64_t fastEnd = length - padding;
    uint8_t * start = (dst + (*offset >> 3)); // byte-offset from bit-offset
    start = (uint8_t*)((uintptr_t)start & ~0x3ULL); // 4-byte aligned
    uint64_t bit_offset = *offset & 31;       // 4-byte aligned bit-offset
    uint32_t* ptr = (uint32_t*)start;         // Pointer to the current window
    uint64_t window = *ptr;

    uint64_t i = 0;
    for(; i < fastEnd; ) {
        // Add symbols until we have 32bits to write out
        while(bit_offset < 32) {
            window |= (uint64_t)encoder->symbols[src[i]] << bit_offset;
            bit_offset += encoder->code_lengths[src[i]];
            window |= (uint64_t)encoder->symbols[src[i+1]] << bit_offset;
            bit_offset += encoder->code_lengths[src[i+1]];
            i += 2;
        }

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

    return PNGENC_SUCCESS;
}

/**
 * Simple reference implementation.
 */
int huffman_encoder_encode_simple(const huffman_encoder * encoder,
                                  const uint8_t * src, uint32_t length,
                                  uint8_t * dst, uint64_t * offset) {
    size_t i = 0;
    for(; i < length; i++) {
        uint8_t current_byte = src[i];
        push_bits(encoder->symbols[current_byte],
                  encoder->code_lengths[current_byte], dst, offset);
    }

    return PNGENC_SUCCESS;
}

int huffman_encoder_get_max_length(const huffman_encoder * encoder) {
    uint32_t max_value = 0;
    for(int i = 0; i < HUFF_MAX_SIZE; i++) {
        max_value = max_u32(max_value, encoder->code_lengths[i]);
    }
    return (int)max_value;
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
 * Post-condition: Current byte is cleared after bit_offset.
 *
 * Current byte:
 * <---Data--------><---Zeroed----->
 * _________________________________
 * | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
 * ¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨
 * bit_offet % 8 ----^
 *
 */
void push_bits(uint64_t bits, uint8_t nbits, uint8_t * data,
               uint64_t * bit_offset) {
    assert(nbits <= 64);

    // fill current byte
    uint64_t offset = *bit_offset;
    data += offset >> 3;
    uint8_t shift = (uint8_t)(offset) & 0x7;
    *bit_offset = offset + nbits;
    int8_t bits_remaining = (int8_t)nbits;
    *data = (*data) | (uint8_t)(bits << shift);
    bits >>= 8 - shift;
    bits_remaining -= 8 - shift;

    // write remaining bytes (or clear next byte in case of -1)
    while(bits_remaining > -1) {
        data++;
        *data = (uint8_t)bits;
        bits >>= 8;
        bits_remaining -= 8;
    }
}
