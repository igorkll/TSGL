#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    tsgl_rgb_565 = 0,
    tsgl_bgr_565,

    tsgl_rgb_888,
    tsgl_bgr_888,
} tsgl_colormode;

typedef uint16_t tsgl_pos;