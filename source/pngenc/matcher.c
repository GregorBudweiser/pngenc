#include "matcher.h"
#include "utils.h"
#include <string.h>
#include <assert.h>

int offset = 0;

void update_entry(tbl_entry * entry, uint32_t position) {
    /*uint32_t i;
    for (i = 0; i < NUM_HASH_ENTRIES; i++) {
        if(entry->positions[i] == 0) {
            entry->positions[i] = position;
            entry->positions[(i+1) % NUM_HASH_ENTRIES] = 0;
            return;
        }
    }*/
    // slightly degrades match quality but performs better (~12% faster)
    entry->positions[offset] = position;
    offset = (offset + 1) % NUM_HASH_ENTRIES;
}

// 3 bytes to 12 bits hash function
uint32_t hash_12b(const uint32_t * data) {
    uint32_t current = *data;
    return ((current >> 12) ^ current) & 0x0FFF;
}

uint32_t match_fwd(const uint8_t * buf, uint32_t max_match_length,
                   uint32_t current_pos, uint32_t proposed_pos) {
    uint32_t i = 0;
    while((i < max_match_length) & (buf[proposed_pos+i] == buf[current_pos+i])) {
        i++;
    }
    return i;
}

uint32_t histogram(const uint8_t * buf, uint32_t length,
                   uint32_t * symbol_histogram, uint32_t *dist_histogram,
                   uint16_t * out_buf, uint32_t * out_length) {
    // Deflate's maximum match
    uint32_t max_match_length = 257;

    tbl_entry hash_table[1 << 12];
    memset(hash_table, 0, (1 << 12)*sizeof(tbl_entry));

    uint32_t out_i = 0;
    uint32_t i, j;
    for(i = 0; i < length; max_match_length = min_u32(max_match_length, length - i)) {
        uint32_t hash = hash_12b((uint32_t*)(buf + i));
        uint32_t best_pos = 0;
        uint32_t best_length = 0;

        // Try all entries for the current hash
        for(j = 0; j < NUM_HASH_ENTRIES; j++) {
            uint32_t proposed_pos = hash_table[hash].positions[j];
            // 16k because we limit to 12 extra bits + 4; 2^4 >= len(extra bits)
            // -1 to fix case where i == proposed_pos
            if (i - proposed_pos - 1 > 16000)
                continue;

            uint32_t match_length = match_fwd(buf, max_match_length,
                                              i, proposed_pos);

            // Pick longest length or shorter distance
            if((match_length > best_length) | ((match_length == best_length)
                                               & (proposed_pos > best_pos))) {
                best_pos = proposed_pos;
                best_length = match_length;
            }
            if(best_length > 2)
                break;
        }

        // Update stuff according to best entry
        update_entry(hash_table + hash, i);

        // Temporary output
        if (best_length > 2) {
            out_i = encode_match_tmp(out_buf, out_i, best_length, i - best_pos,
                                     symbol_histogram, dist_histogram);
            i += best_length;
        } else { // literal
            out_buf[out_i++] = buf[i];
            symbol_histogram[buf[i]]++;
            i++;
        }
    }
    *out_length = out_i;
    return 0;
}

/**
 * Encode a length + distance pair (2-4 temporary symbols used)
 *
 * Symbol selection according to RFC1951, Page 11:
 *
 * https://www.ietf.org/rfc/rfc1951.txt
 * ------------------------------------
 *
 * Match length
 * ------------
 *
 *          Extra               Extra               Extra
 *     Code Bits Length(s) Code Bits Lengths   Code Bits Length(s)
 *     ---- ---- ------    ---- ---- -------   ---- ---- -------
 *      257   0     3       267   1   15,16     277   4   67-82
 *      258   0     4       268   1   17,18     278   4   83-98
 *      259   0     5       269   2   19-22     279   4   99-114
 *      260   0     6       270   2   23-26     280   4  115-130
 *      261   0     7       271   2   27-30     281   5  131-162
 *      262   0     8       272   2   31-34     282   5  163-194
 *      263   0     9       273   3   35-42     283   5  195-226
 *      264   0    10       274   3   43-50     284   5  227-257
 *      265   1  11,12      275   3   51-58     285   0    258
 *      266   1  13,14      276   3   59-66
 *
 *
 * Backward Distance
 * -----------------
 *
 *          Extra           Extra               Extra
 *     Code Bits Dist  Code Bits   Dist     Code Bits Distance
 *     ---- ---- ----  ---- ----  ------    ---- ---- --------
 *       0   0    1     10   4     33-48    20    9   1025-1536
 *       1   0    2     11   4     49-64    21    9   1537-2048
 *       2   0    3     12   5     65-96    22   10   2049-3072
 *       3   0    4     13   5     97-128   23   10   3073-4096
 *       4   1   5,6    14   6    129-192   24   11   4097-6144
 *       5   1   7,8    15   6    193-256   25   11   6145-8192
 *       6   2   9-12   16   7    257-384   26   12  8193-12288
 *       7   2  13-16   17   7    385-512   27   12 12289-16384
 *       8   3  17-24   18   8    513-768   28   13 16385-24576
 *       9   3  25-32   19   8   769-1024   29   13 24577-32768
 *
 * Length:
 * code = 257 + extra_bits*4 + (l - 3)/(2^extra_bits)
 *
 * Backward distance:
 * code = extra_bits*2 + (d - 1)/(2^extra_bits)
 *
 */
uint32_t encode_match_tmp(uint16_t * out, uint32_t out_i,
                          uint32_t match_length, uint32_t bwd_dist,
                          uint32_t * symbol_histogram,
                          uint32_t * dist_histogram) {
    // Encode length
    {
        match_length -= 3;
        uint32_t extra_bits;
#if defined(_MSC_VER)
        extra_bits = __lzcnt(match_length | 1);
#else
        extra_bits = 31 - __builtin_clz(match_length | 1);
#endif
        extra_bits = max_i32(extra_bits-2, 0);
        uint32_t length_code = 257 + (extra_bits << 2) + (match_length >> extra_bits);
        out[out_i] = length_code;
        out_i++;
        symbol_histogram[length_code]++;

        // The part that is dropped during the shifting for code generation
        // contains the "extra bits". We mask them and store them to the next
        // element in the buffer.
        uint32_t mask = (0x1 << extra_bits) - 1;
        out[out_i] = (extra_bits << 8) | (match_length & mask);
        out_i += (int)(extra_bits > 0); // optionally increment (branchless)
    }

    // Encode backward distance
    {
        bwd_dist--;
        uint32_t extra_bits;
#if defined(_MSC_VER)
        extra_bits = __lzcnt(bwd_dist | 1);
#else
        extra_bits = 31 - __builtin_clz(bwd_dist | 1);
#endif
        extra_bits = max_i32(extra_bits-1, 0);
        uint32_t dist_code = (extra_bits << 1) + (bwd_dist >> extra_bits);
        out[out_i] = dist_code;
        assert(dist_code < 32);
        dist_histogram[dist_code]++;
        out_i++;

        // The part that is dropped during the shifting for code generation
        // contains the "extra bits". We mask them and store them to the next
        // element in the buffer.
        uint32_t mask = (0x1 << extra_bits) - 1;
        out[out_i] = (extra_bits << 12) | (bwd_dist & mask);
        out_i += (int)(extra_bits > 0); // optionally increment (branchless)
    }

    return out_i;
}
