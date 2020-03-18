#include "common.h"
#include "string.h"

#include "../source/pngenc/huffman.h"
#include "../source/pngenc/deflate.h"
#include "../source/pngenc/inflate.h"

int test_deflate() {
    uint8_t src[512];
    uint8_t dst[1024];
    memset(src, 0, 512);

    int64_t r_compressed = write_deflate_block_compressed(dst, src, 512, 1);
    int64_t r_uncompressed = write_deflate_block_uncompressed(dst, src, 512, 1);
    ASSERT_TRUE(r_compressed > 0)
    ASSERT_TRUE(r_uncompressed > 0)
    ASSERT_TRUE(r_compressed < r_uncompressed)

    return 0;
}

int test_read_hdr() {
    uint8_t src[512];
    uint8_t dst[1024];
    uint8_t dst2[512];
    for(uint32_t i = 0; i < 512; i++) {
        src[i] = i; //(uint8_t)(i*(i-1));
    }

    write_deflate_block_compressed(dst, src, 512, 1);

    // deflate
    huffman_codec encoder;
    huffman_codec_init(&encoder);
    huffman_codec_add(encoder.histogram, src, 512);
    encoder.histogram[256] = 1; // terminator

    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder, 15, 0.95))
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&encoder))

    huffman_codec code_length_encoder;
    huffman_codec_init(&code_length_encoder);
    huffman_codec_add(code_length_encoder.histogram,
                        encoder.code_lengths, 257);
    // we need to write the zero length for the distance code
    code_length_encoder.histogram[0]++;

    // code length codes limited to 7 bits
    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&code_length_encoder, 7, 0.8))
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&code_length_encoder))


    deflate_codec codec;
    codec.bit_offset = 3; // deflate block header bits
    huffman_codec_init(&codec.huff);
    RETURN_ON_ERROR(read_dynamic_header(dst, 1024, &codec))

    for(uint32_t i = 0; i < HUFF_MAX_LITERALS; i++) {
        printf("%d\n", i);
        ASSERT_EQUAL(codec.huff.code_lengths[i], encoder.code_lengths[i])
        ASSERT_EQUAL(codec.huff.codes[i], encoder.codes[i])
    }

    RETURN_ON_ERROR(decode_data_full(&codec, dst, 1024, dst2, 512))

    ASSERT_EQUAL(memcmp(src, dst2, 512), 0)
    return 0;
}

int test_push_bits() {
    uint8_t buf[3] = { 0, 0, 0 };
    uint64_t bit_offset = 0;

    // put 5 times three bits into buffer/stream
    for(int i = 0; i < 5; i++) {
        push_bits(0x7, 3, buf, &bit_offset);
    }
    // put one more bit into buffer (for a total of 16 bits)
    push_bits(0x1, 1, buf, &bit_offset);

    // test if we have 16 bits set to one
    ASSERT_TRUE(buf[0] == 0xFF)
    ASSERT_TRUE(buf[1] == 0xFF)
    ASSERT_TRUE(buf[2] == 0x0)

    return 0;
}

int test_pop_bits() {
    uint8_t buf[8] = { 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF };
    uint64_t bit_offset = 0;

    // test if we can read each byte correctly
    ASSERT_TRUE(pop_bits(8, buf, &bit_offset) == 0x0)
    ASSERT_TRUE(pop_bits(8, buf, &bit_offset) == 0xFF)
    ASSERT_TRUE(pop_bits(8, buf, &bit_offset) == 0x0)

    bit_offset = 4;
    ASSERT_TRUE(pop_bits(8, buf, &bit_offset) == 0xF0)
    ASSERT_TRUE(pop_bits(8, buf, &bit_offset) == 0x0F)

    bit_offset = 4;
    ASSERT_TRUE(pop_bits(16, buf, &bit_offset) == 0x0FF0)

    bit_offset = 4;
    ASSERT_TRUE(pop_bits(31, buf, &bit_offset) == 0x0FF00FF0)

    bit_offset = 12;
    ASSERT_TRUE(pop_bits(31, buf, &bit_offset) == 0x700FF00F)

    bit_offset = 0;
    for(int i = 0; i < 10; i++) {
        push_bits(i % 7, 3, buf, &bit_offset);
    }

    bit_offset = 0;
    for(int i = 0; i < 10; i++) {
        ASSERT_TRUE(i % 7 == pop_bits(3, buf, &bit_offset))
    }

    return 0;
}

int unit_deflate(int argc, char* argv[]) {
    UNUSED(argc)
    UNUSED(argv)

    RETURN_ON_ERROR(test_pop_bits())
    RETURN_ON_ERROR(test_push_bits())
    RETURN_ON_ERROR(test_deflate())
    RETURN_ON_ERROR(test_read_hdr())

    return 0; // TODO
}

