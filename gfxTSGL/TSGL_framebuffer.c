#include "TSGL.h"
#include "TSGL_framebuffer.h"
#include "TSGL_color.h"
#include "TSGL_spi.h"
#include <esp_heap_caps.h>
#include <esp_err.h>
#include <esp_log.h>

const char* TAG = "TSGL_framebuffer";

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



esp_err_t tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height, int64_t caps) {
    framebuffer->black = tsgl_color_raw(TSGL_BLACK, colormode);
    framebuffer->colorsize = tsgl_colormodeSizes[colormode];
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->defaultWidth = width;
    framebuffer->defaultHeight = height;
    framebuffer->rotation = 0;
    framebuffer->colormode = colormode;
    framebuffer->buffersize = width * height * framebuffer->colorsize;
    if (caps == 0) {
        framebuffer->buffer = malloc(framebuffer->buffersize);
    } else {
        framebuffer->buffer = heap_caps_malloc(framebuffer->buffersize, caps);
        if (framebuffer->buffer == NULL) {
            ESP_LOGW(TAG, "failed to allocate framebuffer with caps. attempt to allocate without caps");
            framebuffer->buffer = malloc(framebuffer->buffersize);
        }
    }
    if (framebuffer->buffer == NULL) {
        ESP_LOGE(TAG, "failed to allocate framebuffer: %ix%ix%i", width, height, framebuffer->colorsize);
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "framebuffer has been successfully allocated: %ix%ix%i", width, height, framebuffer->colorsize);
        return ESP_OK;
    }
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


// graphic

void tsgl_framebuffer_set(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
    if (!_pointInFrame(framebuffer, x, y)) return;
    size_t index = _getBufferIndex(framebuffer, x, y);
    for (uint8_t i = 0; i < framebuffer->colorsize; i++) {
        framebuffer->buffer[index + i] = color.arr[i];
    }
}

void tsgl_framebuffer_fill(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
    for (tsgl_pos ix = x; ix < x + width; ix++) {
        for (tsgl_pos iy = y; iy < y + height; iy++) {
            tsgl_framebuffer_set(framebuffer, ix, iy, color);
        }
    }
}

void tsgl_framebuffer_rect(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
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

void tsgl_framebuffer_clear(tsgl_framebuffer* framebuffer, tsgl_rawcolor color) {
    for (tsgl_pos x = 0; x < framebuffer->width; x++) {
        for (tsgl_pos y = 0; y < framebuffer->height; y++) {
            tsgl_framebuffer_set(framebuffer, x, y, color);
        }
    }
}

tsgl_rawcolor tsgl_framebuffer_get(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    if (!_pointInFrame(framebuffer, x, y)) return framebuffer->black;
    size_t index = _getBufferIndex(framebuffer, x, y);
    tsgl_rawcolor rawcolor;
    for (uint8_t i = 0; i < framebuffer->colorsize; i++) {
        rawcolor.arr[i] = framebuffer->buffer[index + i];
    }
    return rawcolor;
}