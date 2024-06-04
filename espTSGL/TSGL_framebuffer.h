#pragma once
#include "TSGL.h"

typedef struct {
    void* buffer;
    tsgl_pos width;
    tsgl_pos height;
    tsgl_colormode colormode;
} tsgl_framebuffer;

void tsgl_drawPixel(tsgl_pos x, tsgl_pos y, tsgl_color color);