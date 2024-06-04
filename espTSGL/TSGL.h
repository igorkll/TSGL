#pragma once
#include "stdint.h"

typedef enum {
    tsgl_rgb_565,
    tsgl_bgr_565
} tsgl_colormode;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} tsgl_color;

typedef uint16_t tsgl_pos;