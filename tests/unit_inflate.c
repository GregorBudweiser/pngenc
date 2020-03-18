#include "common.h"
#include "../source/pngenc/inflate.h"

/**
 * Can be used to generate the unit test function "test_get_length()".
 * Check table in RFC1951 on page 11 as a reference.
 */
void generate_test_get_length() {
    for(uint32_t i = 265; i < 285; i++) {
        uint8_t num_extra_bits = get_num_extra_bits(i);
        uint32_t max_bits = num_extra_bits > 0 ? (1u << num_extra_bits) - 1 : 0;

        // Output similar to RFC1951 table on page 11
        //printf("%d: %d %d-%d\n", i, (uint32_t)num_extra_bits, get_length(0, num_extra_bits, i), get_length(max_bits, num_extra_bits, i));

        printf("ASSERT_EQUAL(get_length(0, %d, %d), %d)\n", (uint32_t)num_extra_bits, i, get_length(0, num_extra_bits, i));
        printf("ASSERT_EQUAL(get_length(%d, %d, %d), %d)\n", max_bits, (uint32_t)num_extra_bits, i, get_length(max_bits, num_extra_bits, i));
    }
}

int test_get_length() {
    ASSERT_EQUAL(get_length(0, 1, 265), 11)
    ASSERT_EQUAL(get_length(1, 1, 265), 12)
    ASSERT_EQUAL(get_length(0, 1, 266), 13)
    ASSERT_EQUAL(get_length(1, 1, 266), 14)
    ASSERT_EQUAL(get_length(0, 1, 267), 15)
    ASSERT_EQUAL(get_length(1, 1, 267), 16)
    ASSERT_EQUAL(get_length(0, 1, 268), 17)
    ASSERT_EQUAL(get_length(1, 1, 268), 18)
    ASSERT_EQUAL(get_length(0, 2, 269), 35)
    ASSERT_EQUAL(get_length(3, 2, 269), 38)
    ASSERT_EQUAL(get_length(0, 2, 270), 39)
    ASSERT_EQUAL(get_length(3, 2, 270), 42)
    ASSERT_EQUAL(get_length(0, 2, 271), 43)
    ASSERT_EQUAL(get_length(3, 2, 271), 46)
    ASSERT_EQUAL(get_length(0, 2, 272), 47)
    ASSERT_EQUAL(get_length(3, 2, 272), 50)
    ASSERT_EQUAL(get_length(0, 3, 273), 99)
    ASSERT_EQUAL(get_length(7, 3, 273), 106)
    ASSERT_EQUAL(get_length(0, 3, 274), 107)
    ASSERT_EQUAL(get_length(7, 3, 274), 114)
    ASSERT_EQUAL(get_length(0, 3, 275), 115)
    ASSERT_EQUAL(get_length(7, 3, 275), 122)
    ASSERT_EQUAL(get_length(0, 3, 276), 123)
    ASSERT_EQUAL(get_length(7, 3, 276), 130)
    ASSERT_EQUAL(get_length(0, 4, 277), 259)
    ASSERT_EQUAL(get_length(15, 4, 277), 274)
    ASSERT_EQUAL(get_length(0, 4, 278), 275)
    ASSERT_EQUAL(get_length(15, 4, 278), 290)
    ASSERT_EQUAL(get_length(0, 4, 279), 291)
    ASSERT_EQUAL(get_length(15, 4, 279), 306)
    ASSERT_EQUAL(get_length(0, 4, 280), 307)
    ASSERT_EQUAL(get_length(15, 4, 280), 322)
    ASSERT_EQUAL(get_length(0, 5, 281), 643)
    ASSERT_EQUAL(get_length(31, 5, 281), 674)
    ASSERT_EQUAL(get_length(0, 5, 282), 675)
    ASSERT_EQUAL(get_length(31, 5, 282), 706)
    ASSERT_EQUAL(get_length(0, 5, 283), 707)
    ASSERT_EQUAL(get_length(31, 5, 283), 738)
    ASSERT_EQUAL(get_length(0, 5, 284), 739)
    ASSERT_EQUAL(get_length(31, 5, 284), 770)
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
    return 0;
}

