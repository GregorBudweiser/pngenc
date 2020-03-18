#include "common.h"
#include "../source/pngenc/huffman.h"
#include "string.h"

static const int W = 1920;
static const int H = 1080;
static const int C = 3;

void perf_add(const uint8_t * src, uint8_t * dst) {
    huffman_codec encoder;
    huffman_codec_init(&encoder);

    int i, j;
    printf("skimm through memory\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        const int end = C*W*H/8;
        for(j = 0; j < end; j++) {
            volatile register uint64_t tmp = ((uint64_t*)src)[j];
            UNUSED(tmp);
        }
        TIMING_END_MB(6);
    }

    printf("optimized\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        huffman_codec_add(encoder.histogram, src, C*W*H);
        TIMING_END_MB((double)(W*H*C)/1e6);
    }

    printf("simple\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        huffman_encoder_add_simple(encoder.histogram, src, C*W*H);
        TIMING_END_MB((double)(W*H*C)/1e6);
    }

    printf("memset\n");
    for(i = 0; i < 5; i++) {
        TIMING_START;
        memset(dst, 0, C*W*H);
        TIMING_END_MB((double)(W*H*C)/1e6);
    }
}

uint8_t decode_symbol(uint32_t i, uint16_t bits_remaining,
                      const huffman_codec * encoder) {
    for(uint32_t s = 0; s < HUFF_MAX_SYMBOLS; s++) {
        if(encoder->histogram[s] > 0) {
            uint32_t mask = (1 << encoder->code_lengths[s]) - 1;
            if((i & mask) == encoder->codes[s]
                    && bits_remaining >= encoder->code_lengths[s]) {
                if(s == 256) {
                    return 0;
                }
                return encoder->code_lengths[s];
            }
        }
    }
    return 0;
}

uint8_t decode_symbol2(uint32_t i, uint8_t remaining,
                       const huffman_codec * encoder) {
    uint8_t bits_remaining = remaining;
    uint8_t current;
    uint32_t sym = i;
    while((current = decode_symbol(sym, bits_remaining, encoder)) > 0) {
        sym >>= current;
        bits_remaining -= current;
    }
    return remaining - bits_remaining;
}

int perf_encode(const uint8_t * buf, uint8_t * dst) {

    huffman_codec encoder;
    huffman_codec_init(&encoder);
    huffman_codec_add(encoder.histogram, buf, C*W*H);
    encoder.histogram[256] = 1;
    RETURN_ON_ERROR(huffman_encoder_build_tree_limited(&encoder, 15, 0.8))
    RETURN_ON_ERROR(huffman_encoder_build_codes_from_lengths(&encoder))

    uint64_t offset = 0;
    int i;

    printf("encode_simple\n");
    for(i = 0; i < 5; i++) {
        offset = 0;
        TIMING_START
        memset(dst, 0, W*H*C);
        huffman_encoder_encode_simple(&encoder, buf, C*W*H, dst, &offset);
        TIMING_END
    }

    printf("encode64_3\n");
    for(i = 0; i < 5; i++) {
        offset = 0;
        TIMING_START
        // Does not need memset
        huffman_encoder_encode64_3(&encoder, buf, C*W*H, dst, &offset);
        TIMING_END
        push_bits(encoder.codes[256], encoder.code_lengths[256],
                  dst, &offset);
    }

    printf("Encoded data size: %d\n", (int)(offset));

    //printf("Actual max: %d\n", huffman_encoder_get_max_length(&encoder));

    printf("quick_read\n");
    fflush(stdout);
    uint8_t skip[1 << 15];
    const uint32_t my_max = 1 << 15;
    for(uint32_t i = 0; i < my_max; i++) {
        skip[i] = decode_symbol2(i, 15, &encoder);
    }

    for(i = 0; i < 5; i++) {
        offset = 0;
        TIMING_START;
        // Does not need memset
        const uint32_t * ptr = (const uint32_t*)dst;
        uint64_t window = *ptr++;
        uint8_t bits_remaining = 32;
        for(uint32_t i = 0; i < 6*1000*1000/4; i++)  {
            uint8_t to_skip = 1;
            while(bits_remaining >= 32 && to_skip) {
                uint8_t a = skip[window & ((1 << 15)-1)];
                bits_remaining -= a;
                window >>= a;

                uint8_t c = skip[window & ((1 << 15)-1)];
                bits_remaining -= c;
                window >>= c;

                to_skip = c;
            }
            uint64_t next = ((uint64_t)*ptr++) << bits_remaining;
            window |= next;
            bits_remaining += 32;
            if(to_skip == 0) {
                // compute where we are pointing to now
                uint64_t offs = ((const uint8_t*)ptr - dst)*8;
                offs -= bits_remaining;
                // don't forget the terminator symbol:
                // we didnt count that before (skip[256] = 0)
                offs += encoder.code_lengths[256];
                printf("ended at bit #%d\n", (int)offs);
                break;
            }
        }
        TIMING_END;
    }

    return 0;
}

int perf_huffman(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    uint8_t * src;
    uint8_t * dst;
    {
        FILE * file = fopen("data.bin", "rb");
        if(file == 0) {
            printf("Could not find raw image data input.\n");
            printf("Please provide binary image data in the file 'data.bin'.\n");
            printf("Expecting 1920x1080x3 bytes of data.\n");
            return 0;
        }
        src = (uint8_t*)malloc(W*H*C);
        dst = (uint8_t*)malloc(W*H*C*2);
        fread(src, C, W*H, file);
        fclose(file);
    }

    perf_add(src, dst);

    for(int i = C*W*H-1; i >= C; i--) {
        src[i] -= src[i-C];
    }

    RETURN_ON_ERROR(perf_encode(src, dst));

    free(src);
    free(dst);
    return 0;
}

