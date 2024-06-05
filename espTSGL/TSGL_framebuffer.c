#include "TSGL.h"
#include "TSGL_framebuffer.h"
#include "TSGL_color.h"
#include <esp_heap_caps.h>

const uint8_t tsgl_colormode_sizes[] = {2, 2, 2, 2, 3, 3};
static const tsgl_color _black = {
    .r = 0,
    .g = 0,
    .b = 0
};

static tsgl_pos _rotateX(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    switch (framebuffer->rotation) {
        case 0:
            return x;
        case 1:
            return framebuffer->defaultWidth - y - 1;
        case 2:
            return framebuffer->defaultWidth - x - 1;
        case 3:
            return y;
        default:
            return -1;
    }
}

static tsgl_pos _rotateY(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    switch (framebuffer->rotation) {
        case 0:
            return y;
        case 1:
            return x;
        case 2:
            return framebuffer->defaultHeight - y - 1;
        case 3:
            return framebuffer->defaultHeight - x - 1;
        default:
            return -1;
    }
}

static size_t _getBufferIndex(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    return (_rotateX(framebuffer, x, y) + (_rotateY(framebuffer, x, y) * framebuffer->defaultWidth)) * framebuffer->colorsize;
}

static bool _pointInFrame(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    return x >= 0 && y >= 0 && x < framebuffer->width && y < framebuffer->height;
}

bool tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_framebuffer_colormode colormode, tsgl_pos width, tsgl_pos height) {
    framebuffer->colorsize = tsgl_colormode_sizes[colormode];
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->defaultWidth = width;
    framebuffer->defaultHeight = height;
    framebuffer->rotation = 0;
    framebuffer->colormode = colormode;
    framebuffer->buffersize = width * height * framebuffer->colorsize;
    framebuffer->buffer = heap_caps_malloc(framebuffer->buffersize, MALLOC_CAP_DMA);
    return framebuffer->buffer != NULL;
}

void tsgl_framebuffer_free(tsgl_framebuffer* framebuffer) {
    free(framebuffer->buffer);
}

void tsgl_framebuffer_rotate(tsgl_framebuffer* framebuffer, uint8_t rotation) {
    framebuffer->rotation = rotation % 4;
    switch (framebuffer->rotation) {
        case 0:
        case 2:
            framebuffer->width = framebuffer->defaultWidth;
            framebuffer->height = framebuffer->defaultHeight;
            break;
        case 1:
        case 3:
            framebuffer->width = framebuffer->defaultHeight;
            framebuffer->height = framebuffer->defaultWidth;
            break;
    }
}

void tsgl_framebuffer_set(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_color color) {
    if (!_pointInFrame(framebuffer, x, y)) return;
    size_t index = _getBufferIndex(framebuffer, x, y);
    switch (framebuffer->colormode) {
        case tsgl_framebuffer_rgb565_le : {
            uint16_t color565 = tsgl_color_to565(color);
            framebuffer->buffer[index++] = color565 % 256;
            framebuffer->buffer[index] = color565 >> 8;
            break;
        }

        case tsgl_framebuffer_rgb565_be : {
            uint16_t color565 = tsgl_color_to565(color);
            framebuffer->buffer[index++] = color565 >> 8;
            framebuffer->buffer[index] = color565 % 256;
            break;
        }

        case tsgl_framebuffer_bgr565_le : {
            uint16_t color565 = tsgl_color_to565(tsgl_color_pack(color.b, color.g, color.r));
            framebuffer->buffer[index++] = color565 % 256;
            framebuffer->buffer[index] = color565 >> 8;
            break;
        }

        case tsgl_framebuffer_bgr565_be : {
            uint16_t color565 = tsgl_color_to565(tsgl_color_pack(color.b, color.g, color.r));
            framebuffer->buffer[index++] = color565 >> 8;
            framebuffer->buffer[index] = color565 % 256;
            break;
        }

        case tsgl_framebuffer_rgb888 : {
            framebuffer->buffer[index++] = color.r;
            framebuffer->buffer[index++] = color.g;
            framebuffer->buffer[index] = color.b;
            break;
        }

        case tsgl_framebuffer_bgr888 : {
            framebuffer->buffer[index++] = color.b;
            framebuffer->buffer[index++] = color.g;
            framebuffer->buffer[index] = color.r;
            break;
        }
    }
}

tsgl_color tsgl_framebuffer_get(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    if (!_pointInFrame(framebuffer, x, y)) return _black;
    size_t index = _getBufferIndex(framebuffer, x, y);
    switch (framebuffer->colormode) {
        case tsgl_framebuffer_rgb565_le : {
            return tsgl_color_from565(framebuffer->buffer[index] + (framebuffer->buffer[index+1] << 8));
        }

        case tsgl_framebuffer_rgb565_be : {
            return tsgl_color_from565((framebuffer->buffer[index] << 8) + framebuffer->buffer[index+1]);
        }

        case tsgl_framebuffer_bgr565_le : {
            tsgl_color color = tsgl_color_from565(framebuffer->buffer[index] + (framebuffer->buffer[index+1] << 8));
            uint8_t t = color.b;
            color.b = color.r;
            color.r = t;
            return color;
        }

        case tsgl_framebuffer_bgr565_be : {
            tsgl_color color = tsgl_color_from565((framebuffer->buffer[index] << 8) + framebuffer->buffer[index+1]);
            uint8_t t = color.b;
            color.b = color.r;
            color.r = t;
            return color;
        }

        case tsgl_framebuffer_rgb888 : {
            tsgl_color color;
            color.r = framebuffer->buffer[index++];
            color.g = framebuffer->buffer[index++];
            color.b = framebuffer->buffer[index];
            return color;
        }

        case tsgl_framebuffer_bgr888 : {
            tsgl_color color;
            color.b = framebuffer->buffer[index++];
            color.g = framebuffer->buffer[index++];
            color.r = framebuffer->buffer[index];
            return color;
        }
    }
    return _black;
}

void tsgl_framebuffer_fill(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_color color) {
    for (tsgl_pos ix = x; ix < x + width; ix++) {
        for (tsgl_pos iy = y; iy < y + height; iy++) {
            tsgl_framebuffer_set(framebuffer, ix, iy, color);
        }
    }
}

void tsgl_framebuffer_rect(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_color color) {
    tsgl_pos endX = (x + width) - 1;
    tsgl_pos endY = (y + height) - 1;

    for (tsgl_pos ix = x + 1; ix < endX; ix++) {
        tsgl_framebuffer_set(framebuffer, ix, y, color);
        tsgl_framebuffer_set(framebuffer, ix, endY, color);
    }

    for (tsgl_pos iy = y; iy < y + height; iy++) {
        tsgl_framebuffer_set(framebuffer, x, iy, color);
        tsgl_framebuffer_set(framebuffer, endX, iy, color);
    }
}

void tsgl_framebuffer_clear(tsgl_framebuffer* framebuffer, tsgl_color color) {
    for (tsgl_pos x = 0; x < framebuffer->width; x++) {
        for (tsgl_pos y = 0; y < framebuffer->height; y++) {
            tsgl_framebuffer_set(framebuffer, x, y, color);
        }
    }
}