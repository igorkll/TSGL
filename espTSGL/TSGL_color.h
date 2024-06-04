#pragma once
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} tsgl_color;

tsgl_color tsgl_color_pack(uint8_t r, uint8_t g, uint8_t b);
uint16_t tsgl_color_to565(tsgl_color color);
tsgl_color tsgl_color_from565(uint16_t color);
tsgl_color tsgl_color_combine(float v, tsgl_color color1, tsgl_color color2);