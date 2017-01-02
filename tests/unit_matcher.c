#include "common.h"
#include "../source/pngenc/matcher.h"
#include <string.h>
#include <malloc.h>

int test_hash() {
    const int N = 10;
    uint8_t buf[10];
    uint32_t i = 0;
    for(i = 0; i < N; i++) {
        buf[i] = i;
    }

    // Hash function should differ in such simple cases
    for(i = 0; i < N-1; i++) {
        ASSERT_TRUE(hash_12b((const uint32_t*)(buf+i)) != hash_12b((const uint32_t*)(buf+i+1)));
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

int test_encode_tmp_length_code() {
    uint16_t * out = (uint16_t*)malloc(4*sizeof(uint16_t));
    uint32_t * hist_sym = (uint32_t*)malloc(285*sizeof(uint32_t));
    uint32_t * hist_dist = (uint32_t*)malloc(30*sizeof(uint32_t));
    memset(hist_sym, 0, 285*sizeof(uint32_t));
    memset(hist_dist, 0, 30*sizeof(uint32_t));

    // shortest matches
    uint32_t i;
    for(i = 3; i < 11; i++) {
        uint32_t pos = encode_match_tmp(out, 0, i, 1, hist_sym, hist_dist);
        ASSERT_TRUE(out[0] == 257 - 3 + i);
        ASSERT_TRUE(pos == 2); // length + dist
    }

    // all matches with dist + extra bits following (+1 temporary symbol)
    for(i = 11; i < 258; i++) {
        uint32_t pos = encode_match_tmp(out, 0, i, 1, hist_sym, hist_dist);
        ASSERT_TRUE(pos == 3); // lenght + extra + dist
    }

    // 1 extrabit match
    encode_match_tmp(out, 0, 11, 1, hist_sym, hist_dist);
    ASSERT_TRUE(out[0] == 265);

    // 2 extrabits match
    encode_match_tmp(out, 0, 19, 1, hist_sym, hist_dist);
    ASSERT_TRUE(out[0] == 269);

    // 3 extrabits match
    encode_match_tmp(out, 0, 35, 1, hist_sym, hist_dist);
    ASSERT_TRUE(out[0] == 273);

    // 4 extrabits match
    encode_match_tmp(out, 0, 67, 1, hist_sym, hist_dist);
    ASSERT_TRUE(out[0] == 277);

    // 5 extrabits match
    encode_match_tmp(out, 0, 131, 1, hist_sym, hist_dist);
    ASSERT_TRUE(out[0] == 281);

    // longest match
    encode_match_tmp(out, 0, 257, 1, hist_sym, hist_dist);
    ASSERT_TRUE(out[0] == 284);

    free(out);
    free(hist_sym);
    free(hist_dist);

    return 0;
}

int unit_matcher(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    RETURN_ON_ERROR(test_hash());
    RETURN_ON_ERROR(test_match_simple());
    RETURN_ON_ERROR(test_encode_tmp_length_code());
    return 0;
}

