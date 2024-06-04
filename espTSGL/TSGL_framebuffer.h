#pragma once
#include "TSGL.h"
#include "TSGL_color.h"

typedef struct {
    void* buffer;
    tsgl_pos width;
    tsgl_pos height;
    uint8_t colorsize;
    tsgl_colormode colormode;
} tsgl_framebuffer;

bool tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height);
void tsgl_framebuffer_free(tsgl_framebuffer* framebuffer);
void tsgl_framebuffer_set(tsgl_pos x, tsgl_pos y, tsgl_color color);