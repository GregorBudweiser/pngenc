#include "common.h"
#include "../source/pngenc/matcher.h"
#include <string.h>

int test_hash() {
    const int N = 10;
    uint8_t buf[10];
    uint32_t i = 0;
    for(i = 0; i < N; i++) {
        buf[i] = i;
    }

    // Hash function should differ in such simple cases
    for(i = 0; i < N-1; i++) {
        ASSERT_TRUE(hash_12b(buf+i) != hash_12b(buf+i+1));
    }
    return 0;
}

int test_match_simple() {
    uint8_t buf[6] = { 0, 0, 0, 0, 1, 1 };
    int i;

    // All should match up to max length
    for(i = 0; i < 6; i++)
        ASSERT_TRUE(match_fwd(buf, i, 0, 0) == i);

    // Limited by difference
    ASSERT_TRUE(match_fwd(buf, 6, 0, 1) == 3);

    // No match at all
    ASSERT_TRUE(match_fwd(buf, 6, 0, 4) == 0);
    return 0;
}

int unit_matcher(int argc, char* argv[]) {
    (void)argc;
    (void*)argv;

    /*const uint32_t N = 1024;
    uint8_t buf[1024];
    uint32_t hash_table[1 << 12]; // 4k entries * 4 byte = 16kb table

    memset(buf, 0, N);

    uint32_t i;
    for(i = 0; i < N; i++) {
        uint32_t h = hash_12b(buf[i]);
        uint32_t proposed_match_pos = hash_table[h];
        int match_len = match(buf, proposed_match_pos, i, proposed_match_pos);
        //encode_symbol(match_len, i);
        hash_table[h] = i;
    }*/
    RETURN_ON_ERROR(test_hash());
    RETURN_ON_ERROR(test_match_simple());

    return 0;
}

