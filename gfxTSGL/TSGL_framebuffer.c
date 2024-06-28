#include "TSGL.h"
#include "TSGL_framebuffer.h"
#include "TSGL_color.h"
#include "TSGL_spi.h"
#include "TSGL_gfx.h"
#include "TSGL_font.h"

#include <esp_heap_caps.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include <math.h>

static const char* TAG = "TSGL_framebuffer";

static tsgl_pos _customRotateX(tsgl_framebuffer* framebuffer, uint8_t rotation, tsgl_pos x, tsgl_pos y) {
    switch (rotation) {
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

static tsgl_pos _customRotateY(tsgl_framebuffer* framebuffer, uint8_t rotation, tsgl_pos x, tsgl_pos y) {
    switch (rotation) {
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

static tsgl_pos _rotateX(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    return _customRotateX(framebuffer, framebuffer->realRotation, x, y);
}

static tsgl_pos _rotateY(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    return _customRotateY(framebuffer, framebuffer->realRotation, x, y);
}

static size_t _getRawBufferIndex(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    if (framebuffer->realRotation == 0) {
        return x + (y * framebuffer->rotationWidth);
    } else {
        return _rotateX(framebuffer, x, y) + (_rotateY(framebuffer, x, y) * framebuffer->rotationWidth);
    }
}

static size_t _getBufferIndex(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    return _getRawBufferIndex(framebuffer, x, y) * framebuffer->colorsize;
}

static size_t _rawRotateGetBufferIndex(tsgl_framebuffer* framebuffer, uint8_t rotation, tsgl_pos x, tsgl_pos y) {
    if (rotation == 0) {
        return x + (y * framebuffer->rotationWidth);
    } else {
        return _customRotateX(framebuffer, rotation, x, y) + (_customRotateY(framebuffer, rotation, x, y) * framebuffer->rotationWidth);
    }
}

static size_t _rotateGetBufferIndex(tsgl_framebuffer* framebuffer, uint8_t rotation, tsgl_pos x, tsgl_pos y) {
    return _rawRotateGetBufferIndex(framebuffer, rotation, x, y) * framebuffer->colorsize;
}

static bool _pointInFrame(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    return x >= 0 && y >= 0 && x < framebuffer->width && y < framebuffer->height;
}

static bool _isDenseColor(tsgl_rawcolor color, uint8_t colorsize) {
    uint8_t firstPart = color.arr[0];
    for (size_t i = 1; i < colorsize; i++) {
        if (color.arr[i] != firstPart) {
            return false;
        }
    }
    return true;
}



esp_err_t tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height, int64_t caps) {
    framebuffer->black = tsgl_color_raw(TSGL_BLACK, colormode);
    framebuffer->colorsize = tsgl_colormodeSizes[colormode];
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->defaultWidth = width;
    framebuffer->defaultHeight = height;
    framebuffer->rotationWidth = width;
    framebuffer->rotation = 0;
    framebuffer->realRotation = 0;
    framebuffer->colormode = colormode;
    framebuffer->hardwareRotate = false;
    framebuffer->buffersize = width * height * framebuffer->colorsize;
    double notUsed;
    framebuffer->floatColorsize = modf(framebuffer->colorsize, &notUsed) != 0;
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
        ESP_LOGE(TAG, "failed to allocate framebuffer: %i x %i x %.1f", width, height, framebuffer->colorsize);
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "framebuffer has been successfully allocated: %i x %i x %.1f", width, height, framebuffer->colorsize);
        return ESP_OK;
    }
}

void tsgl_framebuffer_free(tsgl_framebuffer* framebuffer) {
    free(framebuffer->buffer);
}

void tsgl_framebuffer_rotate(tsgl_framebuffer* framebuffer, uint8_t rotation) {
    tsgl_framebuffer_hardwareRotate(framebuffer, rotation);
    framebuffer->realRotation = framebuffer->rotation;
    framebuffer->rotationWidth = framebuffer->defaultWidth;
    framebuffer->hardwareRotate = false;
}

void tsgl_framebuffer_hardwareRotate(tsgl_framebuffer* framebuffer, uint8_t rotation) {
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
    framebuffer->realRotation = 0;
    framebuffer->rotationWidth = framebuffer->width;
    framebuffer->hardwareRotate = true;
}


// graphic

void tsgl_framebuffer_push(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, uint8_t rotation, tsgl_framebuffer* sprite, tsgl_rawcolor transparentColor) {
    tsgl_gfx_push(framebuffer, (TSGL_SET_REFERENCE())tsgl_framebuffer_setWithoutCheck, x, y, rotation, sprite, transparentColor, framebuffer->width, framebuffer->height);
}

void tsgl_framebuffer_line(tsgl_framebuffer* framebuffer, tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke) {
    tsgl_gfx_line(framebuffer, (TSGL_SET_REFERENCE())tsgl_framebuffer_setWithoutCheck, (TSGL_FILL_REFERENCE())tsgl_framebuffer_fill, x1, y1, x2, y2, color, stroke, framebuffer->width, framebuffer->height);
}

void tsgl_framebuffer_set(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
    if (!_pointInFrame(framebuffer, x, y)) return;
    tsgl_framebuffer_setWithoutCheck(framebuffer, x, y, color);
}

void tsgl_framebuffer_setWithoutCheck(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
    switch (framebuffer->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            tsgl_color_444write(_getRawBufferIndex(framebuffer, x, y), framebuffer->buffer, color);
            break;
        
        default: {
            size_t index = _getBufferIndex(framebuffer, x, y);
            for (uint8_t i = 0; i < framebuffer->colorsize; i++) {
                framebuffer->buffer[index + i] = color.arr[i];
            }
            break;
        }
    }
}

void tsgl_framebuffer_fill(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
    if (x < 0) {
        width = width + x;
        x = 0;
    }
    if (y < 0) {
        height = height + y;
        y = 0;
    }
    if (width + x > framebuffer->width) width = framebuffer->width - x;
    if (height + y > framebuffer->height) height = framebuffer->height - y;
    if (width <= 0 || height <= 0) return;
    switch (framebuffer->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            for (tsgl_pos ix = x; ix < x + width; ix++) {
                for (tsgl_pos iy = y; iy < y + height; iy++) {
                    tsgl_color_444write(_getRawBufferIndex(framebuffer, ix, iy), framebuffer->buffer, color);
                }
            }
            break;
        
        default:
            for (tsgl_pos ix = x; ix < x + width; ix++) {
                for (tsgl_pos iy = y; iy < y + height; iy++) {
                    size_t index = _getBufferIndex(framebuffer, ix, iy);
                    for (uint8_t i = 0; i < framebuffer->colorsize; i++) {
                        framebuffer->buffer[index + i] = color.arr[i];
                    }
                }
            }
            break;
    }
}

void tsgl_framebuffer_rect(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke) {
    tsgl_gfx_rect(framebuffer, (TSGL_FILL_REFERENCE())tsgl_framebuffer_fill, x, y, width, height, color, stroke);
}

void tsgl_framebuffer_text(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text) {
    tsgl_font_rasterize(framebuffer, (TSGL_SET_REFERENCE())tsgl_framebuffer_set, x, y, sets, text);
}

void tsgl_framebuffer_clear(tsgl_framebuffer* framebuffer, tsgl_rawcolor color) {
    switch (framebuffer->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            tsgl_color nonRawColor = tsgl_color_uraw(color, framebuffer->colormode);
            if (nonRawColor.r == nonRawColor.g && nonRawColor.g == nonRawColor.b) {
                memset(framebuffer->buffer, color.arr[0], framebuffer->buffersize);
            } else {
                size_t rawBuffersize = framebuffer->width * framebuffer->height;
                for (size_t i = 0; i <= rawBuffersize; i++) {
                    tsgl_color_444write(i, framebuffer->buffer, color);
                }
            }
            break;
        
        default:
            if (_isDenseColor(color, framebuffer->colorsize)) {
                memset(framebuffer->buffer, color.arr[0], framebuffer->buffersize);
            } else {
                for (size_t i = 0; i < framebuffer->buffersize; i += framebuffer->colorsize) {
                    memcpy(framebuffer->buffer + i, color.arr, framebuffer->colorsize);
                }
            }
            break;
    }
}

tsgl_rawcolor tsgl_framebuffer_getWithoutCheck(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    switch (framebuffer->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            return tsgl_color_444read(_getRawBufferIndex(framebuffer, x, y), framebuffer->buffer);
        
        default: {
            size_t index = _getBufferIndex(framebuffer, x, y);
            tsgl_rawcolor rawcolor;
            for (uint8_t i = 0; i < framebuffer->colorsize; i++) {
                rawcolor.arr[i] = framebuffer->buffer[index + i];
            }
            return rawcolor;
        }
    }
}

tsgl_rawcolor tsgl_framebuffer_get(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    if (!_pointInFrame(framebuffer, x, y)) return framebuffer->black;
    return tsgl_framebuffer_getWithoutCheck(framebuffer, x, y);
}

tsgl_rawcolor tsgl_framebuffer_rotationGet(tsgl_framebuffer* framebuffer, uint8_t rotation, tsgl_pos x, tsgl_pos y) {
    switch (framebuffer->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            return tsgl_color_444read(_rawRotateGetBufferIndex(framebuffer, rotation, x, y), framebuffer->buffer);
        
        default: {
            size_t index = _rotateGetBufferIndex(framebuffer, rotation, x, y);
            tsgl_rawcolor rawcolor;
            for (uint8_t i = 0; i < framebuffer->colorsize; i++) {
                rawcolor.arr[i] = framebuffer->buffer[index + i];
            }
            return rawcolor;
        }
    }
}