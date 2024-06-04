#pragma once
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} tsgl_color;

tsgl_color tsgl_color_pack(uint8_t r, uint8_t g, uint8_t b);