#include "huffman.h"
#include "utils.h"
#include "pngenc.h"
#include <string.h>
#include <math.h>
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
    return ((const Symbol*)a)->probability - ((const Symbol*)b)->probability;
}

int compare_by_symbol(const void* a, const void* b) {
    return ((const Symbol*)a)->symbol - ((const Symbol*)b)->symbol;
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

void huffman_encoder_add64(uint32_t * histogram, const uint8_t * symbols,
                           uint32_t length) {
    // padd to 8 bytes
    while(length < 0 && (((size_t)symbols) & 0x7) != 0) {
        histogram[*symbols]++;
        symbols++;
    }

    //__attribute__((aligned(64)))
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

    //__attribute__((aligned(64)))
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
                                       uint8_t limit, double power) {
    // TODO: Compute optimal tree (e.g. use optimal algorithm)
    RETURN_ON_ERROR(huffman_encoder_build_tree(encoder));
    while(huffman_encoder_get_max_length(encoder) > limit) {
        for(int i = 0; i < HUFF_MAX_SIZE; i++) {
            if(encoder->histogram[i]) {
                // nonlinearly reduce weight of nodes to lower max tree depth
                uint32_t reduced = (uint32_t)pow(encoder->histogram[i], power);
                encoder->histogram[i] = max_u32(1, reduced);
            }
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
    uint16_t next_free_symbol_value = HUFF_MAX_SIZE;

    // TODO: remove
    memset(symbols, 0, sizeof(Symbol)*(2*HUFF_MAX_SIZE-1));

    // Push raw symbols
    uint32_t i = 0;
    for( ; i < HUFF_MAX_SIZE; i++) {
        next_free_symbol->probability = encoder->histogram[i];
        next_free_symbol->symbol = (uint16_t)i;
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
    qsort(symbols, next_free_symbol - symbols, sizeof(Symbol),
          &compare_by_symbol);

    // compute symbol table first
    int16_t symbol_table[2*HUFF_MAX_SIZE-1];
    memset(symbol_table, 0, sizeof(int16_t)*(2*HUFF_MAX_SIZE-1));
    uint32_t count = (uint32_t)(next_free_symbol - symbols);
    assert(count <= 2*HUFF_MAX_SIZE-1);
    for(i = 0; i < count; i++) {
        symbol_table[symbols[i].symbol] = i;
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
            encoder->symbols[i] = next_code[len-1];
            next_code[len-1]++;
        }
    }

    // bitflip codes:
    for (i = 0; i < HUFF_MAX_SIZE; i++) {
        uint16_t symbol = encoder->symbols[i] << (16-encoder->code_lengths[i]);
        symbol = (bit_reverse_table_256[symbol & 0xFF] << 8)
               | (bit_reverse_table_256[(symbol & 0xFF00) >> 8]);
        encoder->symbols[i] = symbol;
    }

    return PNGENC_SUCCESS;
}

int huffman_encoder_encode(const huffman_encoder * encoder, const uint8_t * src,
                           uint32_t length, uint8_t * dst, uint64_t * offset) {
    if(sizeof(size_t) == 8) {
        return huffman_encoder_encode64(encoder, src, length, dst, offset);
    } else {
        return huffman_encoder_encode32(encoder, src, length, dst, offset);
    }
}


int huffman_encoder_encode2(const huffman_encoder * encoder, const uint8_t * src,
                            uint32_t length, uint8_t * dst, uint64_t * offset) {
    const uint32_t padding = 64; // 64 bit / min Symbol size
    if(length <= padding) {
        return huffman_encoder_encode_simple(encoder, src, length, dst, offset);
    }

    const size_t fastEnd = length - padding;
    uint64_t positionInBits = *offset;
    size_t i;
    for(i = 0; i < fastEnd; i+=6) {
        uint64_t* ptr = ((uint64_t*)(dst+(positionInBits>>3)));
        uint64_t currentWindow = *ptr;
        uint64_t localPositionInBits = positionInBits & 7;

        // Work on 2 x 3 bytes/symbols: Combine 3 symbols into one register each (max 45 bits)
        uint64_t localWindowA = (uint64_t)(encoder->symbols[src[i]]);
        uint64_t localWindowB = (uint64_t)(encoder->symbols[src[i+3]]);

        uint64_t localBitsA = encoder->code_lengths[src[i]];
        uint64_t localBitsB = encoder->code_lengths[src[i+3]];

        localWindowA |= (uint64_t)(encoder->symbols[src[i+1]]) << localBitsA;
        localWindowB |= (uint64_t)(encoder->symbols[src[i+4]]) << localBitsB;
        localBitsA += encoder->code_lengths[src[i+1]];
        localBitsB += encoder->code_lengths[src[i+4]];

        localWindowA |= (uint64_t)(encoder->symbols[src[i+2]]) << localBitsA;
        localWindowB |= (uint64_t)(encoder->symbols[src[i+5]]) << localBitsB;
        localBitsA += encoder->code_lengths[src[i+2]];
        localBitsB += encoder->code_lengths[src[i+5]];

        // Combine A
        currentWindow |= localWindowA << localPositionInBits;
        localPositionInBits += localBitsA;
        positionInBits = (positionInBits & ~7) + localPositionInBits;
        *ptr = currentWindow;

        // Reload current window
        ptr = ((uint64_t*)(dst+(positionInBits>>3)));
        currentWindow = *ptr;
        localPositionInBits = positionInBits & 7;

        // Combine B
        currentWindow |= localWindowB << localPositionInBits;
        localPositionInBits += localBitsB;
        positionInBits = (positionInBits & ~7) + localPositionInBits;
        *ptr = currentWindow;
    }

    // Padding
    for(; i < length; i++) {
        *((uint64_t*)(dst+(positionInBits>>3))) |= encoder->symbols[src[i]]
                                                << (positionInBits & 0x7);
        positionInBits += encoder->code_lengths[src[i]];
    }

    *offset = positionInBits;
    return PNGENC_SUCCESS;
}


int huffman_encoder_encode64(const huffman_encoder * encoder, const uint8_t * src,
                             uint32_t length, uint8_t * dst, uint64_t * offset) {
    const uint32_t padding = 64; // 64 bit / min Symbol size
    if(length <= padding) {
        return huffman_encoder_encode_simple(encoder, src, length, dst, offset);
    }

    const size_t fastEnd = length - padding;
    uint64_t positionInBits = *offset;
    size_t i;
    for(i = 0; i < fastEnd; ) {
        uint64_t* ptr = ((uint64_t*)(dst+(positionInBits>>3)));
        uint64_t currentWindow = *ptr;
        uint64_t localPositionInBits = positionInBits & 7;
        while(localPositionInBits <= 32) {
            currentWindow |= (uint64_t)(encoder->symbols[src[i]])
                          << localPositionInBits;
            localPositionInBits += encoder->code_lengths[src[i]];
            i++;
            currentWindow |= (uint64_t)(encoder->symbols[src[i]])
                          << localPositionInBits;
            localPositionInBits += encoder->code_lengths[src[i]];
            i++;
        }
        positionInBits = (positionInBits & ~7) + localPositionInBits;
        *ptr = currentWindow;
    }

    // Padding
    for(; i < length; i++) {
        *((uint64_t*)(dst+(positionInBits>>3))) |= encoder->symbols[src[i]]
                                                << (positionInBits & 0x7);
        positionInBits += encoder->code_lengths[src[i]];
    }

    *offset = positionInBits;
    return PNGENC_SUCCESS;
}

int huffman_encoder_encode32(const huffman_encoder * encoder, const uint8_t * src,
                             uint32_t length, uint8_t * dst, uint64_t * offset) {
    const uint32_t padding = 64; // 64 bit / min Symbol size
    if(length <= padding) {
        return huffman_encoder_encode_simple(encoder, src, length, dst, offset);
    }

    const size_t fastEnd = length - padding;
    uint64_t positionInBits = *offset;
    size_t i;
    for(i = 0; i < fastEnd; ) {
        uint32_t* ptr = ((uint32_t*)(dst+(positionInBits>>3)));
        uint32_t currentWindow = *ptr;
        uint32_t localPositionInBits = positionInBits & 7;
        while(localPositionInBits <= 17) {
            currentWindow |= (uint32_t)(encoder->symbols[src[i]])
                          << localPositionInBits;
            localPositionInBits += encoder->code_lengths[src[i]];
            i++;
        }
        positionInBits = (positionInBits & ~7) + localPositionInBits;
        *ptr = currentWindow;
    }

    // Padding
    for(; i < length; i++) {
        *((uint64_t*)(dst+(positionInBits>>3))) |= encoder->symbols[src[i]]
                                                << (positionInBits & 0x7);
        positionInBits += encoder->code_lengths[src[i]];
    }

    *offset = positionInBits;
    return PNGENC_SUCCESS;
}


int huffman_encoder_encode3(const huffman_encoder * encoder,
                            const uint8_t * src, uint32_t length,
                            uint8_t * dst, uint64_t * offset) {
    const uint32_t padding = 32; // 64 bit / min Symbol size
    if(length <= padding) {
        return huffman_encoder_encode_simple(encoder, src, length, dst, offset);
    }

    const uint64_t fastEnd = length - padding;
    uint8_t * start = (dst + (*offset >> 3));
    start = (uint8_t*)((uintptr_t)start & ~0x3); // 4-byte aligned (i.e. 32bit aligned)
    uint64_t positionInBits = *offset & 31;      // Offset modulo 32
    uint32_t* ptr = (uint32_t*)start;            // Pointer to the current window
    uint64_t window = *ptr;

    uint64_t i = 0;
    for(; i < fastEnd; ) {
        // Add symbols until we have 32bits to write out
        while(positionInBits < 32) {
            window |= encoder->symbols[src[i]] << positionInBits;
            positionInBits += encoder->code_lengths[src[i]];
            window |= encoder->symbols[src[i+1]] << positionInBits;
            positionInBits += encoder->code_lengths[src[i+1]];
            i += 2;
        }

        *ptr = (uint32_t)window; // Write out compressed stuff in chunks of 32bits
        window = window >> 32;
        ptr++;
        positionInBits &= 31;
    }


    *ptr = (uint32_t)window; // Write out remaining bits
    *(ptr+1) = 0; // Clear next 64bits to allow subsequent access via 64bit reads
    *(ptr+2) = 0;
    positionInBits += (((uint8_t*)ptr) - dst)*8;

    // Padding
    for(; i < length; i++) {
        *((uint64_t*)(dst+(positionInBits>>3))) |= encoder->symbols[src[i]]
                                                << (positionInBits & 0x7);
        positionInBits += encoder->code_lengths[src[i]];
    }

    *offset = positionInBits;

    return PNGENC_SUCCESS;
}

int huffman_encoder_encode_simple(const huffman_encoder * encoder,
                                  const uint8_t * src, uint32_t length,
                                  uint8_t * dst, uint64_t * offset) {
    uint64_t positionInBits = *offset;
    size_t i = 0;
    for(; i < length; i++) {
        uint8_t current_byte = src[i];
        uint16_t symbol = encoder->symbols[current_byte];
        *((uint64_t*)(dst+(positionInBits>>3))) |= symbol
                                                << (positionInBits & 0x7);
        positionInBits += encoder->code_lengths[src[i]];
    }

    *offset = positionInBits;

    return PNGENC_SUCCESS;
}

int huffman_encoder_encode_full_simple(const huffman_encoder * encoder_hist,
                                       const huffman_encoder * encoder_dist,
                                       const uint16_t * src, uint32_t length,
                                       uint8_t * dst, uint64_t * offset) {
    uint64_t positionInBits = *offset;
    size_t i = 0;
    for(; i < length; i++) {
        uint16_t current_byte = src[i];
        // Handle literals (and literal part of matches)
        {
            uint16_t symbol = encoder_hist->symbols[current_byte];
            *((uint64_t*)(dst+(positionInBits>>3))) |= symbol
                                                    << (positionInBits & 0x7);
            positionInBits += encoder_hist->code_lengths[src[i]];
        }

        // Handle matches
        if(current_byte > 255) { // literal was a length code
            // Handle extra bits of length code
            if(current_byte > 264) {
                i++;
                uint16_t length_extra_bits = src[i];
                uint16_t num_extra_bits = length_extra_bits >> 8;
                uint16_t symbol = length_extra_bits & 0xFF;
                *((uint64_t*)(dst+(positionInBits>>3))) |= symbol
                                                        << (positionInBits & 0x7);
                positionInBits += num_extra_bits;
            }

            // handle distance code
            {
                i++;
                uint16_t dist_code = src[i];
                uint16_t symbol = encoder_dist->symbols[dist_code];
                *((uint64_t*)(dst+(positionInBits>>3))) |= symbol
                                                        << (positionInBits & 0x7);
                positionInBits += encoder_dist->code_lengths[src[i]];

                // Handle extra bits of distance code
                if(dist_code > 3) {
                    i++;
                    const uint32_t mask = (0x1 << 12) - 1;
                    uint16_t dist_extra_bits = src[i];
                    uint16_t num_extra_bits = dist_extra_bits >> 12;
                    uint16_t symbol = dist_extra_bits & mask;
                    *((uint64_t*)(dst+(positionInBits>>3))) |= symbol
                                                            << (positionInBits & 0x7);
                    positionInBits += num_extra_bits;
                }
            }
        }
    }

    *offset = positionInBits;

    return PNGENC_SUCCESS;
}

int huffman_encoder_get_max_length(const huffman_encoder * encoder) {
    uint32_t max_value = 0;
    for(int i = 0; i < HUFF_MAX_SIZE; i++) {
        max_value = max_u32(max_value, encoder->code_lengths[i]);
    }
    return max_value;
}

uint32_t huffman_encoder_get_num_literals(const huffman_encoder * encoder) {
    for(int i = HUFF_MAX_SIZE-1; i >= 0; i--) {
        if(encoder->histogram[i] > 0) {
            return i;
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
