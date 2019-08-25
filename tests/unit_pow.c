#include "common.h"


int test_msb_set() {
    ASSERT_TRUE(msb_set(0) == 0);
    for(uint8_t i = 0; i < 31; i++) {
        ASSERT_TRUE(msb_set(1 << i) == i+1);
    }

    ASSERT_TRUE(msb_set(3) == 2);
    ASSERT_TRUE(msb_set(7) == 3);
    ASSERT_TRUE(msb_set(255) == 8);

    return 0;
}

int test_fast_pow95() {
    for(int i = 0; i < 31; i++) {
        int ref = (int)(float)pow(1 << i, 0.95);
        int approx = fast_pow95(1 << i);
        ASSERT_TRUE(ref == approx);
    }

    for(int i = 2; i < 100*1000; i++) {
        ASSERT_TRUE(fast_pow95(i) < i);
    }

    return 0;
}

int test_fast_pow80() {
    for(int i = 0; i < 31; i++) {
        int ref = (int)(float)pow(1 << i, 0.80);
        int approx = fast_pow80(1 << i);
        ASSERT_TRUE(ref == approx);
    }

    for(int i = 2; i < 100*1000; i++) {
        ASSERT_TRUE(fast_pow80(i) < i);
    }

    return 0;
}


int unit_pow(int argc, char* argv[]) {
    RETURN_ON_ERROR(test_msb_set());
    RETURN_ON_ERROR(test_fast_pow80());
    RETURN_ON_ERROR(test_fast_pow95());

    return 0;
}

