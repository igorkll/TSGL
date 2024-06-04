#include "TSGL.h"
#include "TSGL_framebuffer.h"
#include <stdbool.h>
#include <stdlib.h>

static uint8_t tsgl_colormode_sizes[] = {2, 2};

bool tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height) {
    framebuffer->colorsize = tsgl_colormode_sizes[colormode];
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->defaultWidth = width;
    framebuffer->defaultHeight = height;
    framebuffer->rotation = 0;
    framebuffer->buffer = calloc(width * height, framebuffer->colorsize);
    return false;
}

void tsgl_framebuffer_free(tsgl_framebuffer* framebuffer) {
    free(framebuffer->buffer);
}

void tsgl_framebuffer_set(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_color color) {
    size_t index = x + (y * framebuffer->width);
    framebuffer->buffer[index] = ;
}

tsgl_color tsgl_framebuffer_get(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {

}