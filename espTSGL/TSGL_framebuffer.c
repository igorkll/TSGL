#include "TSGL.h"
#include "TSGL_framebuffer.h"
#include "TSGL_color.h"

static const uint8_t colormode_sizes[] = {2, 2, 2, 2, 3, 3};

bool tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height) {
    framebuffer->colorsize = colormode_sizes[colormode];
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->defaultWidth = width;
    framebuffer->defaultHeight = height;
    framebuffer->rotation = 0;
    framebuffer->colormode = colormode;
    framebuffer->buffer = calloc(width * height, framebuffer->colorsize);
    return false;
}

void tsgl_framebuffer_free(tsgl_framebuffer* framebuffer) {
    free(framebuffer->buffer);
}

void tsgl_framebuffer_set(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_color color) {
    size_t index = (x + (y * framebuffer->width)) * framebuffer->colorsize;
    uint8_t* buffer = (uint8_t*)framebuffer->buffer;
    switch (framebuffer->colormode) {
        case tsgl_rgb_565_le:
            uint16_t color565 = tsgl_color_to565(color);
            buffer[index++] = color565 % 256;
            buffer[index] = color565 >> 8;
            break;
        case tsgl_rgb_565_be:
            uint16_t color565 = tsgl_color_to565(color);
            buffer[index++] = color565 >> 8;
            buffer[index] = color565 % 256;
            break;
        case tsgl_bgr_565_le:
            uint16_t color565 = tsgl_color_to565(tsgl_color_pack(color.b, color.g, color.r));
            buffer[index++] = color565 % 256;
            buffer[index] = color565 >> 8;
            break;
        case tsgl_bgr_565_be:
            uint16_t color565 = tsgl_color_to565(tsgl_color_pack(color.b, color.g, color.r));
            buffer[index++] = color565 >> 8;
            buffer[index] = color565 % 256;
            break;
        case tsgl_rgb_888:
            buffer[index++] = color.r;
            buffer[index++] = color.g;
            buffer[index] = color.b;
            break;
        case tsgl_bgr_888:
            buffer[index++] = color.b;
            buffer[index++] = color.g;
            buffer[index] = color.r;
            break;
    }
}

tsgl_color tsgl_framebuffer_get(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    tsgl_color color = {};
    return color;
}