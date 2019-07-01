#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include "common.h"
#include "../source/pngenc/huffman.h"

#define C 3
#define N 1024*1024
#define CODE_LENGTH_LIMIT 15

// Avoid stack overflow
uint8_t src[N];
uint8_t dst[2*N];
uint8_t ref[2*N];

int test_huffman_build_histogram() {
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);

    memset(src, 0, N);
    src[0] = 1;
    src[10] = 1;
    src[100] = 1;

    huffman_encoder_add(encoder.histogram, src, N);

    ASSERT_TRUE(encoder.histogram[0] == N - 3);
    ASSERT_TRUE(encoder.histogram[1] == 3);
    for(int i = 2; i < 257; i++) {
        ASSERT_TRUE(encoder.histogram[i] == 0);
    }

    return 0;
}

int test_huffman_build_tree() {
    huffman_encoder encoder;

    // Check minimal example
    {
        huffman_encoder_init(&encoder);

        // Build histogram; Note that we have only two symbols: One letter (0) and the terminator symbol (256)
        encoder.histogram[0]++; // Add one letter
        encoder.histogram[256]++; // Add terminator symbol

        // Build tree and make sure we got 1-bit codes for the two symbols (everything else would not be efficient and wrong)
        RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder, CODE_LENGTH_LIMIT, 0.95));
        ASSERT_TRUE(encoder.code_lengths[0] == 1);
        ASSERT_TRUE(encoder.code_lengths[256] == 1);

        // Assert that we have different codes for the two symbols
        RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&encoder));
        ASSERT_TRUE(encoder.symbols[0] == 0);
        ASSERT_TRUE(encoder.symbols[256] == 1);
    }

    // Check order of code lenghts
    {
        huffman_encoder_init(&encoder);

        // Build histogram. Higher symbols have higher count => less bits
        int i;
        for(i = 0; i < HUFF_MAX_SIZE; i++) {
            encoder.histogram[i] = i+1;
        }

        // Build tree
        RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder, CODE_LENGTH_LIMIT, 0.95));
        ASSERT_TRUE(encoder.code_lengths[0] <= CODE_LENGTH_LIMIT);
        for(i = 1; i < HUFF_MAX_SIZE; i++) {
            ASSERT_TRUE(encoder.code_lengths[i] <= CODE_LENGTH_LIMIT);
            ASSERT_TRUE(encoder.code_lengths[i] <= encoder.code_lengths[i-1]);
        }
    }
    return 0;
}

int test_huffman_encode() {
    const int OFFSET = 345;
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);

    // Setup encoder
    int i;
    srand(1234);
    for(i = 0; i < N; i++) {
        src[i] = (uint8_t)rand();
    }
    huffman_encoder_add(encoder.histogram, src, N);
    encoder.histogram[256] = 1;
    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder, CODE_LENGTH_LIMIT, 0.95));
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&encoder));

    // Compute reference using simple shifting algorithm
    uint64_t ref_offset;
    //memset(ref, 0, 2*N);
    ref[0] = 0; // only needs first byte cleared
    ref_offset = OFFSET;
    RETURN_ON_ERROR(huffman_encoder_encode_simple(&encoder, src, N, ref, &ref_offset));

    // Compare optimized version
    {
        uint64_t offset;
        memset(dst + (OFFSET/8+1), 0xFF, 2*N - (OFFSET/8+1));
        dst[0] = 0; // only needs first byte cleared
        offset = OFFSET;
        RETURN_ON_ERROR(huffman_encoder_encode64_3(&encoder, src, N, dst, &offset));

        ASSERT_TRUE(offset == ref_offset);
        ASSERT_TRUE(memcmp(ref, dst, offset / 8) == 0);
    }
    return 0;
}

int test_huffman_encode_multi() {
    huffman_encoder encoder;
    huffman_encoder_init(&encoder);

    // Setup encoder
    int i;
    srand(1234);
    for(i = 0; i < N; i++) {
        src[i] = (uint8_t)rand();
    }

    huffman_encoder_add(encoder.histogram, src, N);
    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder, CODE_LENGTH_LIMIT, 0.95));
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&encoder));

    // Compute reference using simple shifting algorithm
    uint64_t ref_offset;
    memset(ref, 0, 2*N);
    ref_offset = 0;
    RETURN_ON_ERROR(huffman_encoder_encode_simple(&encoder, src, N, ref, &ref_offset));

    // Check we get the same when encoding is split up into two steps
    {
        uint64_t offset;
        memset(dst, 0xFF, 2*N); // Does not need output memory to be cleared
        dst[0] = 0; // Only needs first byte to be cleared/zeroed
        offset = 0;
        RETURN_ON_ERROR(huffman_encoder_encode_simple(&encoder, src, N/2+3, dst, &offset));
        RETURN_ON_ERROR(huffman_encoder_encode_simple(&encoder, src+N/2+3, 20, dst, &offset));
        RETURN_ON_ERROR(huffman_encoder_encode_simple(&encoder, src+N/2+23, N/2-23, dst, &offset));

        ASSERT_TRUE(offset == ref_offset);
        ASSERT_TRUE(memcmp(ref, dst, offset / 8) == 0);
    }

    {
        uint64_t offset;
        memset(dst, 0xFF, 2*N); // Does not need output memory to be cleared
        for(i = 0; i < 4; i++) { // Only needs first four bytes to be cleared
            dst[i] = 0;
        }
        offset = 0;
        RETURN_ON_ERROR(huffman_encoder_encode64_3(&encoder, src, N/2+3, dst, &offset));
        RETURN_ON_ERROR(huffman_encoder_encode64_3(&encoder, src+N/2+3, 20, dst, &offset));
        RETURN_ON_ERROR(huffman_encoder_encode64_3(&encoder, src+N/2+23, N/2-23, dst, &offset));

        ASSERT_TRUE(offset == ref_offset);
        ASSERT_TRUE(memcmp(ref, dst, offset / 8) == 0);
    }
    return 0;
}

int unit_huffman(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    RETURN_ON_ERROR(test_huffman_build_histogram());
    RETURN_ON_ERROR(test_huffman_build_tree());
    RETURN_ON_ERROR(test_huffman_encode());
    RETURN_ON_ERROR(test_huffman_encode_multi());
    // TODO: Handle all the other stuff..
    return 0;
}
