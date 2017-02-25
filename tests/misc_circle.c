#include "common.h"
#include "math.h"
#include "assert.h"
#include <emmintrin.h>
#include <pngenc/pngenc.h>

static const int w = 1920;
static const int h = 1080;
static const int r = 300;
static const int s = 12;
static const int numRadii = 20;

void computePrivate();
void computePrivate2();
void computePrivateShort();
void computePrivateSSE();



#define M_PI 3.142f

void downScale(const float * src, float * dst) {
    //for(int y = 0;)
}

void downScaleHalfU8(const uint8_t * src, uint8_t * dst,
                     uint32_t srcW, uint32_t srcH) {
    uint32_t dstW = srcW/2;
    uint32_t dstH = srcH/2;

    uint32_t y;
//#pragma omp parallel for
    for(y = 1; y < dstH-1; y++) {
        uint8_t * dstRow = dst + y*dstW;
        const uint8_t * srcRow0 = src + srcW*(y*2-1);
        const uint8_t * srcRow1 = src + srcW*y*2;
        const uint8_t * srcRow2 = src + srcW*(y*2+1);
        uint32_t x;
        for(x = 1; x < dstW-1; x++) {
            uint32_t r0 = srcRow0[2*x-1] + (srcRow0[2*x] << 1) + srcRow0[2*x+1];
            uint32_t r1 = srcRow1[2*x-1] + (srcRow1[2*x] << 1) + srcRow1[2*x+1];
            uint32_t r2 = srcRow2[2*x-1] + (srcRow2[2*x] << 1) + srcRow2[2*x+1];
            dstRow[x] = (r0 + (r1 << 1) + r2) >> 4;
        }
    }
}

void downScaleHalfU82(const uint8_t * src, uint8_t * dst,
                      uint32_t srcW, uint32_t srcH) {
    uint32_t dstW = srcW/2;
    uint32_t dstH = srcH/2;

    uint32_t y;
//#pragma omp parallel for
    for(y = 1; y < dstH-1; y++) {
        uint8_t * dstRow = dst + y*dstW;
        const uint8_t * srcRow0 = src + srcW*(y*2-1);
        const uint8_t * srcRow1 = src + srcW*y*2;
        const uint8_t * srcRow2 = src + srcW*(y*2+1);
        uint32_t x;
        for(x = 1; x < dstW-1; x++) {
            uint32_t r0 = srcRow0[2*x-1] + (srcRow0[2*x] << 1) + srcRow0[2*x+1];
            uint32_t r1 = srcRow1[2*x-1] + (srcRow1[2*x] << 1) + srcRow1[2*x+1];
            uint32_t r2 = srcRow2[2*x-1] + (srcRow2[2*x] << 1) + srcRow2[2*x+1];
            dstRow[x] = (r0 + (r1 << 1) + r2) >> 4;
        }
    }
}


int misc_circle(int argc, char* argv[]) {

    uint8_t * src = (uint8_t*)malloc(w*h);
    memset(src, 0xFF, w*h);
    uint8_t * dst = (uint8_t*)malloc(w*h/4);
    memset(dst, 0x0, w*h/4);

    float * data = (float*)malloc(sizeof(float)*w*h);
    memset(data, 0, sizeof(float)*w*h);
    {
        int ccx = w/2;
        int ccy = h/2;
        for (float angle = 0.0f; angle < 360.0f; angle += 0.1f) {
            float sinA = (float) sinf(angle * M_PI / 180.0f);
            float cosA = (float) cosf(angle * M_PI / 180.0f);
            int x = ccx + (int)(r*sinA);
            int y = ccy + (int)(r*cosA);
            data[y*w + x] = 1.0f;
            src[y*w + x] = 0;
        }
    }

    float * data2 = (float*)malloc(sizeof(float)*w*h/4);

    // TODO: Make the test work
    for(int i = 0; i < 10; i++) {
        TIMING_START;
        computePrivate(data, 12, 20);
        TIMING_END("test");
    }
    printf("--\n");
    for(int i = 0; i < 10; i++)
    {
        TIMING_START;
        downScaleHalfU8(src, dst, w, h);
        TIMING_END("test");
    }
    pngenc_image_desc img;
    img.strategy = PNGENC_FULL_COMPRESSION;//PNGENC_HUFFMAN_ONLY_WITH_PNG_ROW_FILTER1;
    img.width = w/2;
    img.height = h/2;
    img.row_stride = w/2;
    img.bit_depth = 8;
    img.num_channels = 1;
    img.data = dst;
    pngenc_write_file(&img, "C:/Users/RTFM2/Desktop/_dst.png");
    img.width = w;
    img.height = h;
    img.row_stride = w;
    img.data = src;
    pngenc_write_file(&img, "C:/Users/RTFM2/Desktop/_src.png");
    return 0;
}

struct CenterCandidate {
    int x, y, r;
};

void computePrivateShort() {
    const int windowSize = 2*s + 1;
    const int numRadii = 20;
    const int cxStart = w/2 - s;
    const int cxEnd = cxStart + windowSize;
    const int cyStart = h/2 - s;
    const int cyEnd = cyStart + windowSize;
    const int rStart = r-numRadii/2;

    uint8_t * data = (uint8_t*)malloc(w*h);
    memset(data, 0, w*h);
    {
        int ccx = w/2;
        int ccy = h/2;
        for (float angle = 0.0f; angle < 360.0f; angle += 0.5f) {
            float sinA = (float) sinf(angle * M_PI / 180.0f);
            float cosA = (float) cosf(angle * M_PI / 180.0f);
            int x = ccx + (int)(r*sinA);
            int y = ccy + (int)(r*cosA);
            assert(x >= 0);
            assert(x < w);
            assert(y >= 0);
            assert(y < h);
            data[y*w + x] = 255;
        }
    }

    struct CenterCandidate best;
    double bestValue = 0;

    double * diff = (double*)malloc(sizeof(double)*numRadii);
    uint16_t * sums = (uint16_t*)malloc(sizeof(uint16_t)*numRadii*windowSize);

    //#pragma omp parallel for
    for (int cy = cyStart; cy < cyEnd; cy++) {

        memset(sums, 0, sizeof(uint16_t)*numRadii*windowSize);
        // limit rEnd to avoid out of bounds
        const int rEnd = r+numRadii/2;//getLimitedRadius(cy);

        // sum all values on the circle approximation for the specified
        // radius r

        for (int angle = 0; angle < 256; angle++) {
            float sinA = (float) sinf(angle * M_PI / 180.0f);
            float cosA = (float) cosf(angle * M_PI / 180.0f);

            // Avoid float -> int conversions in innermost loop..
            int sinI = (int) (sinA * (1 << 16));
            int cosI = (int) (cosA * (1 << 16));

            for (int r = rStart; r < rEnd; r++) {
                uint16_t * lines = sums + ((r - rStart)*numRadii);
                int y = cy + ((r * cosI) >> 16);
                y = max_i32(0, min_i32(h - 1, y));
                for (int cx = cxStart; cx < cxEnd; cx++) {
                    int x = cx + ((r * sinI) >> 16);
                    lines[cx - cxStart] += data[y*w + x];
                }
            }
        }

//#pragma omp critical
        for (int cx = 0; cx < windowSize; cx++) {
            for (int idxR = 1; idxR < numRadii; idxR++) {
                diff[idxR] = max_i32(0, sums[idxR*numRadii + cx] - sums[(idxR - 1)*numRadii + cx]);
            }

            // Winner of smoothed data
            //ArrayResult rs = Convolver.convolveMax(diff, kernel, null);
            for (int idxR = 1; idxR < numRadii; idxR++) {
                if (bestValue < diff[idxR]) {
                    bestValue = diff[idxR];
                    best.x = cx + cxStart;
                    best.y = cy;
                    best.r = (idxR + rStart);
                }
            }
        }
    }
    printf("(%d, %d), %d\n", best.x, best.y, best.r);
}


void computePrivateSSE(const float * data) {
    const int windowSize = 2*s + 1;
    const int numRadii = 20;
    const int cxStart = w/2 - windowSize/2;
    const int cxEnd = cxStart + windowSize;
    const int cyStart = h/2 - windowSize/2;
    const int cyEnd = cyStart + windowSize;
    const int rStart = r-numRadii/2;

    struct CenterCandidate best;
    double bestValue = 0;

    double * diff = (double*)malloc(sizeof(double)*numRadii);
    float * sums = (float*)malloc(sizeof(float)*numRadii*windowSize);

    //#pragma omp parallel for
    for (int cy = cyStart; cy < cyEnd; cy++) {

        memset(sums, 0, sizeof(float)*numRadii*windowSize);
        // limit rEnd to avoid out of bounds
        const int rEnd = r+numRadii/2;//getLimitedRadius(cy);

        // sum all values on the circle approximation for the specified
        // radius r
        for (int r = rStart; r < rEnd; r++) {
            float * lines = sums + ((r - rStart)*numRadii);

            __m128 xmm0 = _mm_set1_ps(0.0f);
            __m128 xmm1 = _mm_set1_ps(0.0f);
            __m128 xmm2 = _mm_set1_ps(0.0f);
            __m128 xmm3 = _mm_set1_ps(0.0f);
            __m128 xmm4 = _mm_set1_ps(0.0f);

            for (int angle = 0; angle < 360; angle++) {
                float sinA = (float) sinf(angle * M_PI / 180.0f);
                float cosA = (float) cosf(angle * M_PI / 180.0f);

                // Avoid float -> int conversions in innermost loop..
                int sinI = (int) (sinA * (1 << 16));
                int cosI = (int) (cosA * (1 << 16));

                int y = cy + ((r * cosI) >> 16);
                int x = cxStart + ((r * sinI) >> 16);
                y = max_i32(0, min_i32(h - 1, y));
                const float * ptr = data + (y*w + x);
                xmm0 = _mm_add_ps(xmm0, _mm_loadu_ps(ptr)); // unaligned for now
                xmm1 = _mm_add_ps(xmm1, _mm_loadu_ps(ptr+4));
                xmm2 = _mm_add_ps(xmm2, _mm_loadu_ps(ptr+8));
                xmm3 = _mm_add_ps(xmm3, _mm_loadu_ps(ptr+12));
                xmm4 = _mm_add_ps(xmm4, _mm_loadu_ps(ptr+16));
            }
            _mm_store_ps(lines, xmm0);
            _mm_store_ps(lines+4, xmm1);
            _mm_store_ps(lines+8, xmm2);
            _mm_store_ps(lines+12, xmm3);
            _mm_store_ps(lines+16, xmm4);
        }

        for (int cx = 0; cx < windowSize; cx++) {
            for (int idxR = 1; idxR < numRadii; idxR++) {
                diff[idxR] = max_f32(0.0f, sums[idxR*numRadii + cx] - sums[(idxR - 1)*numRadii + cx]);
            }

            // Winner of smoothed data
            //ArrayResult rs = Convolver.convolveMax(diff, kernel, null);
            for (int idxR = 1; idxR < numRadii; idxR++) {
                if (bestValue < diff[idxR]) {
                    bestValue = diff[idxR];
                    best.x = cx + cxStart;
                    best.y = cy;
                    best.r = (idxR + rStart);
                }
            }
        }
    }
    printf("(%d, %d), %d\n", best.x, best.y, best.r);
}

void computePrivate2(const float * data) {
    const int windowSize = 2*s + 1;
    const int numRadii = 20;
    const int cxStart = w/2 - s;
    const int cxEnd = cxStart + windowSize;
    const int cyStart = h/2 - s;
    const int cyEnd = cyStart + windowSize;
    const int rStart = r-numRadii/2;

    struct CenterCandidate best;
    double bestValue = 0;

    double * diff = (double*)malloc(sizeof(double)*numRadii);
    float * sums = (float*)malloc(sizeof(float)*numRadii*windowSize);

    //#pragma omp parallel for
    for (int cy = cyStart; cy < cyEnd; cy++) {

        memset(sums, 0, sizeof(float)*numRadii*windowSize);
        // limit rEnd to avoid out of bounds
        const int rEnd = r+numRadii/2;//getLimitedRadius(cy);

        // sum all values on the circle approximation for the specified
        // radius r

        for (int r = rStart; r < rEnd; r++) {
            float * lines = sums + ((r - rStart)*numRadii);

            for (int angle = 0; angle < 360; angle++) {
                float sinA = (float) sinf(angle * M_PI / 180.0f);
                float cosA = (float) cosf(angle * M_PI / 180.0f);

                // Avoid float -> int conversions in innermost loop..
                int sinI = (int) (sinA * (1 << 16));
                int cosI = (int) (cosA * (1 << 16));

                int y = cy + ((r * cosI) >> 16);
                y = max_i32(0, min_i32(h - 1, y));
                for (int cx = cxStart; cx < cxEnd; cx++) {
                    int x = cx + ((r * sinI) >> 16);
                    lines[cx - cxStart] += data[y*w + x];
                }
            }
        }

//#pragma omp critical
        for (int cx = 0; cx < windowSize; cx++) {
            for (int idxR = 1; idxR < numRadii; idxR++) {
                diff[idxR] = max_f32(0.0f, sums[idxR*numRadii + cx] - sums[(idxR - 1)*numRadii + cx]);
            }

            // Winner of smoothed data
            //ArrayResult rs = Convolver.convolveMax(diff, kernel, null);
            for (int idxR = 1; idxR < numRadii; idxR++) {
                if (bestValue < diff[idxR]) {
                    bestValue = diff[idxR];
                    best.x = cx + cxStart;
                    best.y = cy;
                    best.r = (idxR + rStart);
                }
            }
        }
    }
    printf("(%d, %d), %d\n", best.x, best.y, best.r);
}

void computePrivate(const float * data, int s, int nr) {
    const int windowSize = 2*s + 1;
    const int numRadii = nr;
    const int cxStart = w/2 - s;
    const int cxEnd = cxStart + windowSize;
    const int cyStart = h/2 - s;
    const int cyEnd = cyStart + windowSize;
    const int rStart = r-numRadii/2;

    struct CenterCandidate best;
    double bestValue = 0;

    double * diff = (double*)malloc(sizeof(double)*numRadii);
    float * sums = (float*)malloc(sizeof(float)*numRadii*windowSize);

    //#pragma omp parallel for
    for (int cy = cyStart; cy < cyEnd; cy++) {

        memset(sums, 0, sizeof(float)*numRadii*windowSize);
        // limit rEnd to avoid out of bounds
        const int rEnd = r+numRadii/2;//getLimitedRadius(cy);

        // sum all values on the circle approximation for the specified
        // radius r

        for (int angle = 0; angle < 360; angle++) {
            float sinA = (float) sinf(angle * M_PI / 180.0f);
            float cosA = (float) cosf(angle * M_PI / 180.0f);

            // Avoid float -> int conversions in innermost loop..
            int sinI = (int) (sinA * (1 << 16));
            int cosI = (int) (cosA * (1 << 16));

            for (int r = rStart; r < rEnd; r++) {
                float * lines = sums + ((r - rStart)*numRadii);
                int y = cy + ((r * cosI) >> 16);
                y = max_i32(0, min_i32(h - 1, y));
                for (int cx = cxStart; cx < cxEnd; cx++) {
                    int x = cx + ((r * sinI) >> 16);
                    lines[cx - cxStart] += data[y*w + x];
                }
            }
        }

//#pragma omp critical
        for (int cx = 0; cx < windowSize; cx++) {
            for (int idxR = 1; idxR < numRadii; idxR++) {
                diff[idxR] = max_f32(0.0f, sums[idxR*numRadii + cx] - sums[(idxR - 1)*numRadii + cx]);
            }

            // Winner of smoothed data
            //ArrayResult rs = Convolver.convolveMax(diff, kernel, null);
            for (int idxR = 1; idxR < numRadii; idxR++) {
                if (bestValue < diff[idxR]) {
                    bestValue = diff[idxR];
                    best.x = cx + cxStart;
                    best.y = cy;
                    best.r = (idxR + rStart);
                }
            }
        }
    }
    printf("(%d, %d), %d\n", best.x, best.y, best.r);
}
