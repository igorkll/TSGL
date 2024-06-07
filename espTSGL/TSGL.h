#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef int16_t tsgl_pos;

extern const uint8_t tsgl_colormodeSizes[];

typedef enum {
    tsgl_rgb565_le,
    tsgl_rgb565_be,
    tsgl_bgr565_le,
    tsgl_bgr565_be,
    tsgl_rgb888,
    tsgl_bgr888,
} tsgl_colormode;