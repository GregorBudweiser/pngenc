#include "common.h"
#include "../source/pngenc/inflate.h"
#include "../source/pngenc/zlib.h"
#include <string.h>

int test_inflate_raw() {
    zlib_codec zcodec;
    zcodec.deflate.bit_offset = 0;
    zcodec.buf_size = 128; // TODO: in or out?
    zcodec.state = UNINITIALIZED;
    huffman_codec_init(&zcodec.deflate.huff);
    const uint8_t data[] = {
        // 1024 zeroes compressed (deflate block created by QT's qCompress())
        0x78, 0xda, 0x63, 0x60, 0x18, 0x05, 0xa3, 0x60, 0x14, 0x8c, 0x54, 0x00, 0x00, 0x04, 0x00, 0x00, 0x01
    };

    uint8_t decompressed[1024];
    memset(decompressed, 0xFF, sizeof(decompressed));


    int64_t result = decode_zlib_stream(decompressed, sizeof(decompressed), data, sizeof(data), &zcodec);
    if(result < 0) {
        return (int)result;
    }

    for(int i = 0; i < 1024; i++) {
        ASSERT_EQUAL(decompressed[i], 0);
    }
    return 0;
}

/**
 * Can be used to generate the unit test function "test_get_length()".
 * Check table in RFC1951 on page 11 as a reference.
 */
void generate_test_get_length() {
    for(uint32_t i = 265; i < 285; i++) {
        uint8_t num_extra_bits = get_num_extra_bits(i);
        uint32_t max_bits = num_extra_bits > 0 ? (1u << num_extra_bits) - 1 : 0;

        // Output similar to RFC1951 table on page 11
        //printf("%d: %d %d-%d\n", i, (uint32_t)num_extra_bits, get_length(0, i), get_length(max_bits, i));

        printf("ASSERT_EQUAL(get_length(0, %d), %d)\n", i, get_length(0, i));
        printf("ASSERT_EQUAL(get_length(%d, %d), %d)\n", max_bits, i, get_length(max_bits, i));
    }
}

int test_get_length() {
    ASSERT_EQUAL(get_length(0, 265), 11)
    ASSERT_EQUAL(get_length(1, 265), 12)
    ASSERT_EQUAL(get_length(0, 266), 13)
    ASSERT_EQUAL(get_length(1, 266), 14)
    ASSERT_EQUAL(get_length(0, 267), 15)
    ASSERT_EQUAL(get_length(1, 267), 16)
    ASSERT_EQUAL(get_length(0, 268), 17)
    ASSERT_EQUAL(get_length(1, 268), 18)
    ASSERT_EQUAL(get_length(0, 269), 19)
    ASSERT_EQUAL(get_length(3, 269), 22)
    ASSERT_EQUAL(get_length(0, 270), 23)
    ASSERT_EQUAL(get_length(3, 270), 26)
    ASSERT_EQUAL(get_length(0, 271), 27)
    ASSERT_EQUAL(get_length(3, 271), 30)
    ASSERT_EQUAL(get_length(0, 272), 31)
    ASSERT_EQUAL(get_length(3, 272), 34)
    ASSERT_EQUAL(get_length(0, 273), 35)
    ASSERT_EQUAL(get_length(7, 273), 42)
    ASSERT_EQUAL(get_length(0, 274), 43)
    ASSERT_EQUAL(get_length(7, 274), 50)
    ASSERT_EQUAL(get_length(0, 275), 51)
    ASSERT_EQUAL(get_length(7, 275), 58)
    ASSERT_EQUAL(get_length(0, 276), 59)
    ASSERT_EQUAL(get_length(7, 276), 66)
    ASSERT_EQUAL(get_length(0, 277), 67)
    ASSERT_EQUAL(get_length(15, 277), 82)
    ASSERT_EQUAL(get_length(0, 278), 83)
    ASSERT_EQUAL(get_length(15, 278), 98)
    ASSERT_EQUAL(get_length(0, 279), 99)
    ASSERT_EQUAL(get_length(15, 279), 114)
    ASSERT_EQUAL(get_length(0, 280), 115)
    ASSERT_EQUAL(get_length(15, 280), 130)
    ASSERT_EQUAL(get_length(0, 281), 131)
    ASSERT_EQUAL(get_length(31, 281), 162)
    ASSERT_EQUAL(get_length(0, 282), 163)
    ASSERT_EQUAL(get_length(31, 282), 194)
    ASSERT_EQUAL(get_length(0, 283), 195)
    ASSERT_EQUAL(get_length(31, 283), 226)
    ASSERT_EQUAL(get_length(0, 284), 227)
    ASSERT_EQUAL(get_length(31, 284), 258)
    return 0;
}

void generate_test_get_dist() {
    for(uint32_t i = 4; i < 30; i++) {
        uint8_t num_extra_bits = get_num_extra_bits_dist(i);
        uint32_t max_bits = num_extra_bits > 0 ? (1u << num_extra_bits) - 1 : 0;

        // Output similar to RFC1951 table on page 11
        //printf("%d: %d %d-%d\n", i, (uint32_t)num_extra_bits, get_dist(0, num_extra_bits, i), get_dist(max_bits, num_extra_bits, i));

        printf("ASSERT_EQUAL(get_dist(0, %d, %d), %d)\n", (uint32_t)num_extra_bits, i, get_dist(0, num_extra_bits, i));
        printf("ASSERT_EQUAL(get_dist(%d, %d, %d), %d)\n", max_bits, (uint32_t)num_extra_bits, i, get_dist(max_bits, num_extra_bits, i));
    }
}

int test_get_dist() {
    ASSERT_EQUAL(get_dist(0, 1, 4), 5)
    ASSERT_EQUAL(get_dist(1, 1, 4), 6)
    ASSERT_EQUAL(get_dist(0, 1, 5), 7)
    ASSERT_EQUAL(get_dist(1, 1, 5), 8)
    ASSERT_EQUAL(get_dist(0, 2, 6), 9)
    ASSERT_EQUAL(get_dist(3, 2, 6), 12)
    ASSERT_EQUAL(get_dist(0, 2, 7), 13)
    ASSERT_EQUAL(get_dist(3, 2, 7), 16)
    ASSERT_EQUAL(get_dist(0, 3, 8), 17)
    ASSERT_EQUAL(get_dist(7, 3, 8), 24)
    ASSERT_EQUAL(get_dist(0, 3, 9), 25)
    ASSERT_EQUAL(get_dist(7, 3, 9), 32)
    ASSERT_EQUAL(get_dist(0, 4, 10), 33)
    ASSERT_EQUAL(get_dist(15, 4, 10), 48)
    ASSERT_EQUAL(get_dist(0, 4, 11), 49)
    ASSERT_EQUAL(get_dist(15, 4, 11), 64)
    ASSERT_EQUAL(get_dist(0, 5, 12), 65)
    ASSERT_EQUAL(get_dist(31, 5, 12), 96)
    ASSERT_EQUAL(get_dist(0, 5, 13), 97)
    ASSERT_EQUAL(get_dist(31, 5, 13), 128)
    ASSERT_EQUAL(get_dist(0, 6, 14), 129)
    ASSERT_EQUAL(get_dist(63, 6, 14), 192)
    ASSERT_EQUAL(get_dist(0, 6, 15), 193)
    ASSERT_EQUAL(get_dist(63, 6, 15), 256)
    ASSERT_EQUAL(get_dist(0, 7, 16), 257)
    ASSERT_EQUAL(get_dist(127, 7, 16), 384)
    ASSERT_EQUAL(get_dist(0, 7, 17), 385)
    ASSERT_EQUAL(get_dist(127, 7, 17), 512)
    ASSERT_EQUAL(get_dist(0, 8, 18), 513)
    ASSERT_EQUAL(get_dist(255, 8, 18), 768)
    ASSERT_EQUAL(get_dist(0, 8, 19), 769)
    ASSERT_EQUAL(get_dist(255, 8, 19), 1024)
    ASSERT_EQUAL(get_dist(0, 9, 20), 1025)
    ASSERT_EQUAL(get_dist(511, 9, 20), 1536)
    ASSERT_EQUAL(get_dist(0, 9, 21), 1537)
    ASSERT_EQUAL(get_dist(511, 9, 21), 2048)
    ASSERT_EQUAL(get_dist(0, 10, 22), 2049)
    ASSERT_EQUAL(get_dist(1023, 10, 22), 3072)
    ASSERT_EQUAL(get_dist(0, 10, 23), 3073)
    ASSERT_EQUAL(get_dist(1023, 10, 23), 4096)
    ASSERT_EQUAL(get_dist(0, 11, 24), 4097)
    ASSERT_EQUAL(get_dist(2047, 11, 24), 6144)
    ASSERT_EQUAL(get_dist(0, 11, 25), 6145)
    ASSERT_EQUAL(get_dist(2047, 11, 25), 8192)
    ASSERT_EQUAL(get_dist(0, 12, 26), 8193)
    ASSERT_EQUAL(get_dist(4095, 12, 26), 12288)
    ASSERT_EQUAL(get_dist(0, 12, 27), 12289)
    ASSERT_EQUAL(get_dist(4095, 12, 27), 16384)
    ASSERT_EQUAL(get_dist(0, 13, 28), 16385)
    ASSERT_EQUAL(get_dist(8191, 13, 28), 24576)
    ASSERT_EQUAL(get_dist(0, 13, 29), 24577)
    ASSERT_EQUAL(get_dist(8191, 13, 29), 32768)
    return 0;
}

int unit_inflate(int argc, char* argv[]) {
    RETURN_ON_ERROR(test_get_length())
    RETURN_ON_ERROR(test_get_dist())
    RETURN_ON_ERROR(test_inflate_raw())
    return 0;
}

