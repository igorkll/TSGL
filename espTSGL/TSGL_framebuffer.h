#pragma once
#include "TSGL.h"
#include "TSGL_color.h"

extern const uint8_t tsgl_framebuffer_colormodeSizes[];

typedef enum {
    tsgl_framebuffer_rgb565_be,
    tsgl_framebuffer_bgr565_be,
    tsgl_framebuffer_rgb565_le,
    tsgl_framebuffer_bgr565_le,
    tsgl_framebuffer_rgb888,
    tsgl_framebuffer_bgr888,
} tsgl_framebuffer_colormode;

typedef struct {
    uint8_t* buffer;
    size_t buffersize;
    tsgl_pos width;
    tsgl_pos height;
    tsgl_pos defaultWidth;
    tsgl_pos defaultHeight;
    uint8_t colorsize;
    uint8_t rotation;
    tsgl_framebuffer_colormode colormode;
} tsgl_framebuffer;

bool tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_framebuffer_colormode colormode, tsgl_pos width, tsgl_pos height, uint32_t caps);
void tsgl_framebuffer_free(tsgl_framebuffer* framebuffer);
void tsgl_framebuffer_rotate(tsgl_framebuffer* framebuffer, uint8_t rotation); //rotates the indexing of the framebuffer and not the framebuffer itself
void tsgl_framebuffer_set(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_color color);
tsgl_color tsgl_framebuffer_get(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y);
void tsgl_framebuffer_fill(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_color color);
void tsgl_framebuffer_rect(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_color color);
void tsgl_framebuffer_clear(tsgl_framebuffer* framebuffer, tsgl_color color);