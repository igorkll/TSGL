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

bool tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height);
void tsgl_framebuffer_free(tsgl_framebuffer* framebuffer);

void tsgl_framebuffer_set(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_color color);
tsgl_color tsgl_framebuffer_get(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y);