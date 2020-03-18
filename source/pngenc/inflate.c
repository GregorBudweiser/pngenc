#include "deflate.h"
#include "huffman.h"
#include "utils.h"
#include "pngenc.h"
#include "decoder.h"
#include "inflate.h"
#include <stdio.h>
#include <string.h>

/**
 * Build symbol lookup table. The table allows to "parse" the next
 * symbol given a stream of bytes such that:
 *    symbol = lut[raw_bits];
 *
 * Or rather:
 *    mask = ((0x1 << max_code_length)-1);
 *    symbol = lut[raw_bits & mask];
 *
 * Or even:
 *    symbol = lut[huffman_code];
 *
 * The symbol then points to the huffman code in
 * the encoder structure:
 *    huffman_code = encoder.codes[symbol];
 */
void build_sym_lut(uint32_t lut_size, const huffman_codec * encoder,
                   uint16_t * sym_lut, uint8_t * skip_bits_lut,
                   uint32_t sym_start, uint32_t sym_end,
                   uint8_t max_code_length) {
    uint32_t ctr = 0;
    uint32_t ctr2 = 0;
    for(uint32_t i = 0; i < lut_size; i++) {
        skip_bits_lut[i] = 0;
        sym_lut[i] = 0xFFFF;

        // find symbol that matches the first n bits
        for(uint32_t sym = sym_start; sym < sym_end; sym++) {
            uint8_t current_code_len = encoder->code_lengths[sym];
            if(current_code_len > 0) { // does the symbol exist in the alphabet?
                uint32_t mask = (1 << current_code_len) - 1;
                if((i & mask) == encoder->codes[sym]) {
                    sym_lut[i] = (uint16_t)sym;
                    skip_bits_lut[i] = current_code_len;
                    ctr += sym > 256 ? 1 : 0;
                    break;
                }
            }
        }
    }

    for(uint32_t sym = sym_start; sym < sym_end; sym++) {
        if(encoder->code_lengths[sym] &&
           sym_lut[encoder->codes[sym]] != sym) {
            printf("Nope: %d -> %d -> %d\n", sym, encoder->codes[sym],
                   sym_lut[encoder->codes[sym]]);
            ctr2++;
        }
    }

    printf("%d/%d; err: %d/%d\n", ctr, lut_size, ctr2, (sym_end - sym_start));
    fflush(stdout);
}


void build_sym_lut2(uint32_t lut_size, const huffman_codec * encoder,
                    uint16_t * sym_lut, uint8_t * skip_bits_lut,
                    uint32_t sym_start, uint32_t sym_end,
                    uint8_t max_code_length) {
    for(uint32_t i = 0; i < lut_size; i++) {
        skip_bits_lut[i] = 0;
        sym_lut[i] = 0xFFFF;
    }

    // find symbol that matches the first n bits
    for(uint32_t sym = sym_start; sym < sym_end; sym++) {
        uint8_t current_code_len = encoder->code_lengths[sym];
        if(current_code_len > 0) { // does the symbol exist in the alphabet?

            const uint32_t mask = (1 << current_code_len) - 1;
            for(uint32_t i = 0; i < lut_size; i++) {
                if((i & mask) == encoder->codes[sym]) {
                    sym_lut[i] = (uint16_t)sym;
                    skip_bits_lut[i] = current_code_len;
                }
            }

        }
    }

    uint32_t err = 0;
    for(uint32_t i = 0; i < lut_size; i++) {
        if(sym_lut[i] == 0xFFFF) {
            err++;
        }
    }
    printf("%d/%d errors\n", err, lut_size);
}


int decode_code_lengths(const huffman_codec * code_length_encoder,
                        deflate_codec * deflate,
                        const uint8_t * src, const uint32_t num_lit_symbols,
                        const uint32_t num_dist_symbols) {
    const uint32_t num_symbols = num_lit_symbols + num_dist_symbols;

    huffman_codec * encoder = &deflate->huff;
    uint8_t offs = deflate->bit_offset & 31ull;
    const uint8_t * data = src + ((deflate->bit_offset & ~31ull) >> 3);  // align to 32bits and convert to byte-offset
    uint8_t skip_code[1 << 7];
    uint16_t sym_idx_lut[1 << 7]; // maps code_length_encoder.symbols to encoder.code_length (i.e. decodes literal code lengths)
    build_sym_lut2(1 << 7, code_length_encoder, sym_idx_lut, skip_code,
                   0, 19, 7); // there are only 18 code lengths, no?

    const uint32_t * ptr = (const uint32_t*)data;
    uint64_t window = *ptr++;
    window >>= offs;
    uint8_t bits_remaining = 32-offs;

    uint32_t idx;
    for(idx = 0; idx < num_symbols; )  {
        if(bits_remaining < 32) {
            uint64_t next = ((uint64_t)*ptr++) << bits_remaining;
            window |= next;
            bits_remaining += 32;
        }
        uint8_t length = skip_code[window & ((1 << 7)-1)];
        encoder->code_lengths[idx] = (uint8_t)sym_idx_lut[window & ((1 << 7)-1)];
        bits_remaining -= length;
        window >>= length;
        deflate->bit_offset += length;

        // handle repeated codes and increment counter accordingly
        switch(encoder->code_lengths[idx]) {
            case 16: {
                const uint8_t last_code = encoder->code_lengths[idx-1];
                uint8_t num_iter = (window & 0x3) + 3;
                window >>= 2;
                bits_remaining -= 2;
                deflate->bit_offset += 2;
                for(uint8_t i = 0; i < num_iter; i++) {
                    encoder->code_lengths[idx++] = last_code;
                }
                break;
            }
            case 17: {
                uint8_t num_iter = (window & 0x7) + 3;
                window >>= 3;
                bits_remaining -= 3;
                deflate->bit_offset += 3;
                for(uint8_t i = 0; i < num_iter; i++) {
                    encoder->code_lengths[idx++] = 0;
                }
                break;
            }
            case 18: {
                uint8_t num_iter = (window & 0x7F) + 11;
                window >>= 7;
                bits_remaining -= 7;
                deflate->bit_offset += 7;
                for(uint8_t i = 0; i < num_iter; i++) {
                    encoder->code_lengths[idx++] = 0;
                }
                break;
            }
            default:
                idx++;
                break;
        }
    }

    printf("idx: %d, hlit+hdist: %d\n", idx, (int)num_symbols);
    fflush(stdout);

    // We decoded literals and distances at once.
    // Now we have to move dist codes to the correct position in the alphabet.
    memcpy(encoder->code_lengths + HUFF_MAX_LITERALS,
           encoder->code_lengths + num_lit_symbols,
           num_dist_symbols*sizeof(uint8_t));
    memset(encoder->code_lengths + num_lit_symbols, 0,
           (HUFF_MAX_LITERALS-num_lit_symbols)*sizeof(uint8_t));
    memcpy(encoder->codes + HUFF_MAX_LITERALS,
           encoder->codes + num_lit_symbols,
           num_dist_symbols*sizeof(uint16_t));
    memset(encoder->codes + num_lit_symbols, 0,
           (HUFF_MAX_LITERALS-num_lit_symbols)*sizeof(uint16_t));

    // TODO: handle incorrect/corrupted input
    return PNGENC_SUCCESS;
}

uint8_t get_num_extra_bits(uint32_t symbol_idx) {
    return (uint8_t)((symbol_idx - 261u)/4u);
}

uint32_t base_value(uint8_t num_extra_bits) {
    return 7u + (((1u << num_extra_bits) - 1u) << 2);
}

uint32_t get_length(uint32_t extra_bits, uint8_t num_extra_bits,
                    uint32_t symbol_idx) {
    return base_value(num_extra_bits)
         + (1u << num_extra_bits)*(symbol_idx - (261u + 4u)) + extra_bits;
}

uint8_t get_num_extra_bits_dist(uint32_t symbol_idx) {
    return (uint8_t)((symbol_idx >> 1) - 1u);
}

uint32_t base_value_dist(uint8_t num_extra_bits) {
    return 5u + (((1u << (num_extra_bits-1u)) - 1u) << 2u);
}

uint32_t get_dist(uint32_t extra_bits, uint8_t num_extra_bits,
                  uint32_t symbol_idx) {
    return base_value_dist(num_extra_bits)
         + (1u << num_extra_bits)*(symbol_idx & 0x1) + extra_bits;
}


int decode_data_full(deflate_codec * deflate,
                     const uint8_t * src, uint32_t src_size,
                     uint8_t * dst, uint32_t dst_size) {

    uint8_t * const dst_old  = dst;

    // literal + length codes
    uint8_t skip_code[1 << 15];
    uint16_t sym_idx_lut[1 << 15];
    build_sym_lut2(1 << 15, &deflate->huff, sym_idx_lut, skip_code,
                   0, HUFF_MAX_LITERALS, 15);

    // dist codes
    uint8_t skip_code_dist[1 << 15];
    uint16_t sym_idx_lut_dist[1 << 15];
    build_sym_lut2(1 << 15, &deflate->huff, sym_idx_lut_dist, skip_code_dist,
                   HUFF_MAX_LITERALS, HUFF_MAX_SYMBOLS, 15);

    const uint64_t start = deflate->bit_offset;

    uint8_t offs = deflate->bit_offset & 31ull;
    const uint8_t * data = src + ((deflate->bit_offset & ~31ull) >> 3);  // align to 32bits and convert to byte-offset
    const uint32_t * ptr = (const uint32_t*)data;
    uint64_t window = *ptr++;
    window >>= offs;
    uint8_t bits_remaining = 32-offs;

    // Todo stop reading if bit_offset goes out of valid range
    uint32_t current_sym_idx = 0;
    uint32_t num_bytes_read = 0;

    uint32_t literals = 0;
    uint32_t match = 0;

    while(1)  { // Terminator symbol / end of stream
        if(bits_remaining < 32) {
            uint64_t next = ((uint64_t)*ptr++) << bits_remaining;
            window |= next;
            bits_remaining += 32;
            num_bytes_read += 4;
        }
        uint8_t length = skip_code[window & ((1 << 15)-1)];
        current_sym_idx = sym_idx_lut[window & ((1 << 15)-1)];

        bits_remaining -= length;
        window >>= length;
        deflate->bit_offset += length;

        if(current_sym_idx > 256) { // do we have a distance code?
            // find out length of backward match/repeat/copy/whatever
            uint32_t len;
            if(current_sym_idx > 284) {
                len = 258;
            } else if(current_sym_idx > 264) {
                // max 5 extra bits
                uint8_t num_extra_bits = get_num_extra_bits(current_sym_idx);
                uint32_t extra_bits = pop_bits(num_extra_bits, src, &deflate->bit_offset);
                bits_remaining -= num_extra_bits;
                window >>= num_extra_bits;
                len = get_length(extra_bits, num_extra_bits, current_sym_idx);
            } else {
                len = current_sym_idx - 254;
            }
            match++;

            if(bits_remaining < 32) {
                uint64_t next = ((uint64_t)*ptr++) << bits_remaining;
                window |= next;
                bits_remaining += 32;
                num_bytes_read += 4;
            }

            // fetch distance
            uint8_t dist_sym_length = skip_code_dist[window & ((1 << 15)-1)];
            uint16_t dist_sym = sym_idx_lut_dist[window & ((1 << 15)-1)] - HUFF_MAX_LITERALS; // TODO: do the minus in the table

            deflate->bit_offset += dist_sym_length;
            bits_remaining -= dist_sym_length;
            window >>= dist_sym_length;

            // fetch extrabits and compute dist
            uint32_t dist;
            if (dist_sym > 3) {
                uint8_t num_extra_bits = get_num_extra_bits_dist(dist_sym);
                uint32_t extra_bits = pop_bits(num_extra_bits, src, &deflate->bit_offset);
                bits_remaining -= num_extra_bits;
                window >>= num_extra_bits;
                dist = get_dist(extra_bits, num_extra_bits, dist_sym);
            } else {
                dist = dist_sym + 1;
            }

            // todo check bounds..
            //printf("backward match with len: %d; dist: %d\n", len, dist);
            //memcpy(dst, dst - dist, len); // <-- does handle overlap which is not what we need
            for(uint32_t i = 0; i < len; i++) {
                dst[i] = dst[(int)i-(int)dist];
            }
            dst += len;
        } else if (current_sym_idx == 256) {
            break;
        } else { // literal
            *dst = (uint8_t)current_sym_idx;
            dst++;
            literals++;
        }
    }

    const uint64_t num_bytes = (deflate->bit_offset - start)/8;

    printf("bytes read: %d\n", (int)num_bytes);
    printf("bytes written: %d\n", (int)(dst - dst_old));
    fflush(stdout);

    return (int)(dst - dst_old);
}



int64_t read_dynamic_header(const uint8_t * src, uint32_t src_size,
                            deflate_codec * deflate) {
    /*
     * From the RFC:
     *
     * We can now define the format of the block:
     *
     *               5 Bits: HLIT, # of Literal/Length codes - 257 (257 - 286)
     *               5 Bits: HDIST, # of Distance codes - 1        (1 - 32)
     *               4 Bits: HCLEN, # of Code Length codes - 4     (4 - 19)
     */
    // 256 literals + 1 termination symbol
    const uint32_t HLIT = 257 + pop_bits(5, src, &deflate->bit_offset);
    // 1 distance code; set it to 0 to signal that we only use literals
    const uint32_t HDIST = 1 + pop_bits(5, src, &deflate->bit_offset);
    // 19 code lengths of the actual code lengths (SEE RFC1951)
    const uint32_t HCLEN = 4 + pop_bits(4, src, &deflate->bit_offset);

    printf("HLIT: %d (%d), HDIST %d (%d), HCLEN: %d (%d)\n",
           (int)HLIT, HLIT-257, (int)HDIST, HDIST-1, (int)HCLEN, HCLEN-4);
    fflush(stdout);

    huffman_codec code_length_encoder;
    huffman_codec_init(&code_length_encoder);

    /*
     *       (HCLEN + 4) x 3 bits: code lengths for the code length
     *           alphabet given just above, in the order: 16, 17, 18,
     *           0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
     *
     *           These code lengths are interpreted as 3-bit integers
     *           (0-7); as above, a code length of 0 means the
     *           corresponding symbol (literal/length or distance code
     *           length) is not used.
     */
    const uint8_t indices[] = {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
    };

    for(uint32_t i = 0; i < HCLEN; i++) {
        code_length_encoder.code_lengths[indices[i]] =
                (uint8_t)pop_bits(3, src, &deflate->bit_offset);
    }

    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths2(
                        code_length_encoder.code_lengths,
                        code_length_encoder.codes, 19))
    RETURN_ON_ERROR(decode_code_lengths(&code_length_encoder, deflate, src, HLIT, HDIST))
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths2(
                        deflate->huff.code_lengths,
                        deflate->huff.codes, (uint32_t)HLIT))
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths2(
                        deflate->huff.code_lengths+HUFF_MAX_LITERALS,
                        deflate->huff.codes+HUFF_MAX_LITERALS, (uint32_t)HDIST))

    return PNGENC_SUCCESS;
}

