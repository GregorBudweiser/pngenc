#include "huffman.h"
#include "utils.h"
#include "pngenc.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

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

void huffman_codec_init(huffman_codec * encoder) {
    memset(encoder, 0, sizeof(huffman_codec));
}

void huffman_codec_add(uint32_t * histogram, const uint8_t * symbols,
                         uint32_t length) {
    if(sizeof(size_t) == 8) {
        huffman_codec_add64(histogram, symbols, length);
    } else {
        huffman_codec_add32(histogram, symbols, length);
    }
}

/**
 * Same as huffman_encoder_add_simple just optimized for 64bit machines.
 */
void huffman_codec_add64(uint32_t * histogram, const uint8_t * symbols,
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


void huffman_codec_add32(uint32_t * histogram, const uint8_t * symbols,
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
int huffman_encoder_build_tree_limited(huffman_codec * encoder,
                                       uint8_t limit, power_coefficient power) {
    // TODO: Compute optimal tree (e.g. use optimal algorithm)
    RETURN_ON_ERROR(huffman_encoder_build_tree(encoder));
    while(huffman_encoder_get_max_length(encoder) > limit) {
        // nonlinearly reduce weight of nodes to lower max tree depth
        switch(power) {
        case POW_80:
            for(int i = 0; i < HUFF_MAX_SYMBOLS; i++) {
                encoder->histogram[i] = fast_pow80(encoder->histogram[i]);
            }
            break;
        case POW_95: // fallthrough
        default:
            for(int i = 0; i < HUFF_MAX_SYMBOLS; i++) {
                encoder->histogram[i] = fast_pow95(encoder->histogram[i]);
            }
            break;
        }
        RETURN_ON_ERROR(huffman_encoder_build_tree(encoder));
    }
    return PNGENC_SUCCESS;
}

int huffman_encoder_build_tree(huffman_codec * encoder) {

    //const uint64_t N_SYMBOLS = HUFF_MAX_SIZE;
    // msvc cant do this because it lacks c99 support?
    //Symbol symbols[2*N_SYMBOLS-1];
    Symbol symbols[2*HUFF_MAX_SYMBOLS-1];
    Symbol * next_free_symbol = symbols;
    int16_t next_free_symbol_value = HUFF_MAX_SYMBOLS;

    // TODO: remove
    memset(symbols, 0, sizeof(Symbol)*(2*HUFF_MAX_SYMBOLS-1));

    // Push raw symbols
    uint32_t i = 0;
    for( ; i < HUFF_MAX_SYMBOLS; i++) {
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
    int16_t symbol_table[2*HUFF_MAX_SYMBOLS-1];
    memset(symbol_table, 0, sizeof(int16_t)*(2*HUFF_MAX_SYMBOLS-1));
    uint32_t count = (uint32_t)(next_free_symbol - symbols);
    assert(count <= 2*HUFF_MAX_SYMBOLS-1);
    for(i = 0; i < count; i++) {
        symbol_table[symbols[i].symbol] = (int16_t)i;
    }

    // Get actual lengths
    assert(num_symbols <= HUFF_MAX_SYMBOLS);
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

int huffman_encoder_build_codes_from_lengths(huffman_codec * encoder) {
    // huffman codes are limited to 15bits
    if(huffman_encoder_get_max_length(encoder) > 15) {
        return PNGENC_ERROR;
    }
    return huffman_encoder_build_codes_from_lengths2(encoder->code_lengths,
                                                     encoder->codes,
                                                     HUFF_MAX_LITERALS);
}

int huffman_encoder_build_codes_from_lengths2(const uint8_t * code_lengths,
                                              uint16_t * symbols, uint32_t n) {
    // Find number of elements per code length
    const uint8_t MAX_BITS = 16;
    uint8_t num_code_lengths[16]; // for msvc... [MAX_BITS];
    memset(num_code_lengths, 0, sizeof(uint8_t)*MAX_BITS);

    uint32_t i;
    for(i = 0; i < n; i++) {
        assert(code_lengths[i] <= MAX_BITS);
        num_code_lengths[code_lengths[i]]++;
    }

    // Generate base code for each length (see deflate rfc 1951)
    uint32_t next_code[16];
    memset(next_code, 0, sizeof(uint32_t)*MAX_BITS);
    uint32_t code = 0;
    uint32_t bits;
    num_code_lengths[0] = 0;
    for(bits = 1; bits < MAX_BITS; bits++) {
        code = (code + num_code_lengths[bits-1]) << 1;
        next_code[bits] = code;
    }

    // Generate final codes
    for (i = 0;  i < n; i++) {
        uint8_t len = code_lengths[i];
        symbols[i] = (uint16_t)next_code[len];
        next_code[len]++;
    }

    // bitflip codes:
    for (i = 0; i < n; i++) {
        uint32_t symbol = (uint32_t)symbols[i] << (16-code_lengths[i]);
        symbol = ((uint32_t)bit_reverse_table_256[symbol & 0xFF] << 8)
               | ((uint32_t)bit_reverse_table_256[(symbol & 0xFF00) >> 8]);
        symbols[i] = (uint16_t)symbol;
    }
    return PNGENC_SUCCESS;
}


int huffman_encoder_build_codes_from_lengths3(const uint8_t * code_lengths,
                                              uint16_t * symbols, uint32_t n) {
    // Find number of elements per code length
    const uint8_t MAX_BITS = 7;
    uint8_t num_code_lengths[7]; // for msvc... [MAX_BITS];
    memset(num_code_lengths, 0, sizeof(uint8_t)*MAX_BITS);

    uint32_t i;
    for(i = 0; i < n; i++) {
        assert(code_lengths[i] <= MAX_BITS);
        num_code_lengths[code_lengths[i]]++;
    }

    // Generate base code for each length (see deflate rfc 1951)
    uint32_t next_code[7];
    memset(next_code, 0, sizeof(uint32_t)*MAX_BITS);
    uint32_t code = 0;
    uint32_t bits;
    for(bits = 1; bits < MAX_BITS; bits++) {
        code = (code + num_code_lengths[bits-1]) << 1;
        next_code[bits] = code;
    }

    // Generate final codes
    for (i = 0;  i <= n; i++) {
        uint8_t len = code_lengths[i];
        if (len > 0) {
            symbols[i] = (uint16_t)next_code[len-1];
            next_code[len-1]++;
        }
    }

    // bitflip codes:
    for (i = 0; i < n; i++) {
        uint32_t symbol = (uint32_t)symbols[i] << (16-code_lengths[i]);
        symbol = ((uint32_t)bit_reverse_table_256[symbol & 0xFF] << 8)
               | ((uint32_t)bit_reverse_table_256[(symbol & 0xFF00) >> 8]);
        symbols[i] = (uint16_t)symbol;
    }

    return PNGENC_SUCCESS;
}

int huffman_encoder_encode(const huffman_codec * encoder, const uint8_t * src,
                           uint32_t length, uint8_t * dst, uint64_t * offset) {
    // Cannot assume dst to be zeroed!
    return huffman_encoder_encode64_3(encoder, src, length, dst, offset);
}

/**
 * This function does not have unaligned memory writes like the other encode
 * functions. Puts less pressure on MMU and scales better when parallelized.
 */
int huffman_encoder_encode64_3(const huffman_codec * encoder,
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
            window |= (uint64_t)encoder->codes[src[i]] << bit_offset;
            bit_offset += encoder->code_lengths[src[i]];
            window |= (uint64_t)encoder->codes[src[i+1]] << bit_offset;
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
        push_bits(encoder->codes[current_byte],
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
int huffman_encoder_encode_simple(const huffman_codec * encoder,
                                  const uint8_t * src, uint32_t length,
                                  uint8_t * dst, uint64_t * offset) {
    size_t i = 0;
    for(; i < length; i++) {
        uint8_t current_byte = src[i];
        push_bits(encoder->codes[current_byte],
                  encoder->code_lengths[current_byte], dst, offset);
    }

    return PNGENC_SUCCESS;
}

int huffman_encoder_get_max_length(const huffman_codec * encoder) {
    uint32_t max_value = 0;
    for(int i = 0; i < HUFF_MAX_SYMBOLS; i++) {
        max_value = max_u32(max_value, encoder->code_lengths[i]);
    }
    return (int)max_value;
}

uint32_t huffman_encoder_get_num_literals(const huffman_codec * encoder) {
    for(int i = HUFF_MAX_SYMBOLS-1; i >= 0; i--) {
        if(encoder->histogram[i] > 0) {
            return (uint32_t)i;
        }
    }
    return 0;
}

void huffman_encoder_print(const huffman_codec * encoder, const char * name) {
    printf("Huffman code for %s\n", name);
    for(int i = 0; i < HUFF_MAX_LITERALS; i++) {
        if(encoder->code_lengths[i]) {
            printf(" > Literal Code %d: %d bits (%d)\n", i,
                   encoder->code_lengths[i], encoder->codes[i]);
        }
    }
    for(int i = HUFF_MAX_LITERALS; i < HUFF_MAX_SYMBOLS; i++) {
        if(encoder->code_lengths[i]) {
            printf(" > Distance Code %d: %d bits (%d)\n", i,
                   encoder->code_lengths[i], encoder->codes[i]);
        }
    }
    fflush(stdout);
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


uint32_t pop_bits(uint8_t nbits, const uint8_t * src, uint64_t * bit_offset) {
    const uint32_t mask = (0x1u << nbits) - 1;
    src += (*bit_offset) / 8;
    const uint8_t shift = (uint8_t)((*bit_offset) % 8);
    *bit_offset += nbits;
    int8_t bits_remaining = (int8_t)nbits;
    uint32_t result = (*src) >> shift;
    bits_remaining -= 8 - shift;

    // read remaining bits
    while(bits_remaining > 0) {
        uint32_t current_byte = *++src;
        result |= current_byte << (nbits - bits_remaining);
        bits_remaining -= 8;
    }
    return result & mask;
}
