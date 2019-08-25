#pragma once
#include <stdint.h>

typedef enum _power_coefficient {
    POW_80,
    POW_95
} power_coefficient;

uint32_t msb_set(uint32_t value);
uint32_t fast_pow95(uint32_t value);
uint32_t fast_pow80(uint32_t value);

