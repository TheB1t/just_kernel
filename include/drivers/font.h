#pragma once

#include <klibc/stdlib.h>

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t* data;
} font_t;

extern font_t font8x8_basic;