#include "TSGL.h"
#include "TSGL_framebuffer.h"
#include <stdbool.h>
#include <stdlib.h>

static uint8_t tsgl_colormode_sizes[] = {2, 2};

bool tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height) {
    framebuffer->colorsize = tsgl_colormode_sizes[colormode];
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->buffer = calloc(width * height, framebuffer->colorsize);
    return false;
}

void tsgl_framebuffer_free(tsgl_framebuffer* framebuffer) {
    free(framebuffer->buffer);
}