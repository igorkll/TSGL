#pragma once
#include "TSGL.h"
#include "TSGL_color.h"

typedef struct {
    void* buffer;
    tsgl_pos width;
    tsgl_pos height;
    tsgl_pos defaultWidth;
    tsgl_pos defaultHeight;
    uint8_t colorsize;
    uint8_t rotation;
    tsgl_colormode colormode;
} tsgl_framebuffer;

void tsgl_display_init();
void tsgl_display_free();