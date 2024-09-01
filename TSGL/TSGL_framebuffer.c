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
#define _NEG_HUGE -2147483648 
#define _HUGE 2147483647

inline static tsgl_pos _customRotateX(tsgl_framebuffer* framebuffer, uint8_t rotation, tsgl_pos x, tsgl_pos y) {
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

inline static tsgl_pos _customRotateY(tsgl_framebuffer* framebuffer, uint8_t rotation, tsgl_pos x, tsgl_pos y) {
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

inline static tsgl_pos _rotateX(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    return _customRotateX(framebuffer, framebuffer->realRotation, x, y);
}

inline static tsgl_pos _rotateY(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    return _customRotateY(framebuffer, framebuffer->realRotation, x, y);
}

inline static size_t _getRawBufferIndex(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    if (framebuffer->realRotation == 0) {
        return x + (y * framebuffer->rotationWidth);
    } else {
        return _rotateX(framebuffer, x, y) + (_rotateY(framebuffer, x, y) * framebuffer->rotationWidth);
    }
}

inline static size_t _getRawHorBufferIndex(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    if (framebuffer->realRotation == 0) {
        return x + ((y / 8) * framebuffer->rotationWidth);
    } else {
        return _rotateX(framebuffer, x, y) + ((_rotateY(framebuffer, x, y) / 8) * framebuffer->rotationWidth);
    }
}

inline static uint8_t _getHorOffset(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    switch (framebuffer->realRotation) {
        case 1:
            return x % 8;

        case 2:
            return 7 - (y % 8);

        case 3:
            return 7 - (x % 8);
    }

    return y % 8;
}

inline static size_t _getBufferIndex(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    return _getRawBufferIndex(framebuffer, x, y) * framebuffer->colorsize;
}

inline static size_t _rawRotateGetBufferIndex(tsgl_framebuffer* framebuffer, uint8_t rotation, tsgl_pos x, tsgl_pos y) {
    if (rotation == 0) {
        return x + (y * framebuffer->rotationWidth);
    } else {
        return _customRotateX(framebuffer, rotation, x, y) + (_customRotateY(framebuffer, rotation, x, y) * framebuffer->rotationWidth);
    }
}

inline static size_t _rotateGetBufferIndex(tsgl_framebuffer* framebuffer, uint8_t rotation, tsgl_pos x, tsgl_pos y) {
    return _rawRotateGetBufferIndex(framebuffer, rotation, x, y) * framebuffer->colorsize;
}

inline static bool _pointInFrame(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    return x >= framebuffer->viewport_minX && y >= framebuffer->viewport_minY && x < framebuffer->viewport_maxX && y < framebuffer->viewport_maxY;
}

inline static bool _isDenseColor(tsgl_rawcolor color, uint8_t colorsize) {
    uint8_t firstPart = color.arr[0];
    for (size_t i = 1; i < colorsize; i++) {
        if (color.arr[i] != firstPart) {
            return false;
        }
    }
    return true;
}

inline static void _doubleSet(tsgl_framebuffer* framebuffer, size_t index, tsgl_rawcolor color) {
    uint16_t* var = (uint16_t*)(framebuffer->buffer + index);
    *var = *((uint16_t*)(&color.arr[0]));
}

inline static void _monoWrite(size_t index, uint8_t offset, uint8_t* buffer, tsgl_rawcolor color) {
    if (color.arr[0]) {
        buffer[index] |= 1 << offset;
    } else {
        buffer[index] &= ~(1 << offset);
    }
}

inline static void _444write(size_t rawindex, uint8_t* buffer, tsgl_rawcolor color) {
    size_t index = rawindex * 1.5;
    if ((rawindex & 1) == 0) {
        buffer[index] = color.arr[0];
        buffer[index+1] = (color.arr[1] & 0b11110000) | (buffer[index+1] & 0b1111);
    } else {
        buffer[index] = (color.arr[1] & 0b00001111) | (buffer[index] & 0b11110000);
        buffer[index+1] = color.arr[2];
    }
}

inline static tsgl_rawcolor _444read(size_t rawindex, uint8_t* buffer) {
    size_t index = rawindex * 1.5;
    uint8_t v0 = 0;
    uint8_t v1 = 0;
    uint8_t v2 = 0;
    if ((rawindex & 1) == 0) {
        v0 = buffer[index] >> 4;
        v1 = buffer[index] & 0b1111;
        v2 = buffer[index+1] >> 4;
    } else {
        v0 = buffer[index] & 0b1111;
        v1 = buffer[index+1] >> 4;
        v2 = buffer[index+1] & 0b1111;
    }
    tsgl_rawcolor result = {
        .arr = {
            (v0 << 4) | v1,
            (v2 << 4) | v0,
            (v1 << 4) | v2
        }
    };
    return result;
}


esp_err_t tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height, int64_t caps) {
    memset(framebuffer, 0, sizeof(tsgl_framebuffer));
    framebuffer->black = tsgl_color_raw(TSGL_BLACK, colormode);
    framebuffer->colorsize = tsgl_colormodeSizes[colormode];
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->defaultWidth = width;
    framebuffer->defaultHeight = height;
    framebuffer->rotationWidth = width;
    framebuffer->colormode = colormode;
    framebuffer->buffersize = width * height * framebuffer->colorsize;
    double notUsed;
    framebuffer->floatColorsize = modf(framebuffer->colorsize, &notUsed) != 0;
    framebuffer->buffer = tsgl_malloc(framebuffer->buffersize, caps);
    tsgl_framebuffer_resetChangedArea(framebuffer);
    tsgl_framebuffer_clrViewport(framebuffer);
    if (framebuffer->buffer == NULL) {
        ESP_LOGE(TAG, "failed to allocate framebuffer: %i x %i x %.3f", width, height, framebuffer->colorsize);
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "framebuffer has been successfully allocated: %i x %i x %.3f", width, height, framebuffer->colorsize);
        return ESP_OK;
    }
}

tsgl_framebuffer* tsgl_framebuffer_new(tsgl_colormode colormode, tsgl_pos width, tsgl_pos height, int64_t caps) {
    tsgl_framebuffer* framebuffer = malloc(sizeof(tsgl_framebuffer));
    if (tsgl_framebuffer_init(framebuffer, colormode, width, height, caps) != ESP_OK) {
        free(framebuffer);
        return NULL;
    }
    framebuffer->heap = true;
    return framebuffer;
}

void tsgl_framebuffer_free(tsgl_framebuffer* framebuffer) {
    free(framebuffer->buffer);
    if (framebuffer->heap) free(framebuffer);
}

inline void tsgl_framebuffer_resetChangedArea(tsgl_framebuffer* framebuffer) {
    framebuffer->changed = false;

    framebuffer->changedFrom = _HUGE;
    framebuffer->changedTo = _NEG_HUGE;

    framebuffer->changedLeft = TSGL_POS_MAX;
    framebuffer->changedRight = TSGL_POS_MIN;
    framebuffer->changedUp = TSGL_POS_MAX;
    framebuffer->changedDown = TSGL_POS_MIN;
}

inline void tsgl_framebuffer_allChangedArea(tsgl_framebuffer* framebuffer) {
    framebuffer->changedFrom = 0;
    framebuffer->changedTo = framebuffer->buffersize - 1;

    framebuffer->changedLeft = 0;
    framebuffer->changedRight = framebuffer->width - 1;
    framebuffer->changedUp = 0;
    framebuffer->changedDown = framebuffer->height - 1;
}

inline void tsgl_framebuffer_updateChangedAreaIndex(tsgl_framebuffer* framebuffer, int32_t index) {
    if (index < framebuffer->changedFrom) framebuffer->changedFrom = index;
    if (index > framebuffer->changedTo) framebuffer->changedTo = index;
}

inline void tsgl_framebuffer_updateChangedAreaXY(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    if (x < framebuffer->changedLeft) framebuffer->changedLeft = x;
    if (x > framebuffer->changedRight) framebuffer->changedRight = x;
    if (y < framebuffer->changedUp) framebuffer->changedUp = y;
    if (y > framebuffer->changedDown) framebuffer->changedDown = y;
}

inline void tsgl_framebuffer_clrViewport(tsgl_framebuffer* framebuffer) {
    tsgl_framebuffer_setViewport(framebuffer, 0, 0, framebuffer->width, framebuffer->height);
    framebuffer->viewport = false;
}

inline void tsgl_framebuffer_setViewport(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    framebuffer->viewport = x != 0 || y != 0 || width != framebuffer->width || height != framebuffer->height;
    framebuffer->viewport_minX = x;
    framebuffer->viewport_minY = y;
    framebuffer->viewport_maxX = x + width;
    framebuffer->viewport_maxY = y + height;
}

inline void tsgl_framebuffer_rotate(tsgl_framebuffer* framebuffer, uint8_t rotation) {
    tsgl_framebuffer_hardwareRotate(framebuffer, rotation);
    framebuffer->realRotation = framebuffer->rotation;
    framebuffer->rotationWidth = framebuffer->defaultWidth;
    framebuffer->hardwareRotate = false;
    framebuffer->softwareRotate = framebuffer->rotation != 0;
}

inline void tsgl_framebuffer_hardwareRotate(tsgl_framebuffer* framebuffer, uint8_t rotation) {
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
    framebuffer->softwareRotate = false;
    tsgl_framebuffer_clrViewport(framebuffer);
}


// graphic

inline void tsgl_framebuffer_push(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_sprite* sprite) {
    tsgl_gfx_push(framebuffer, (TSGL_SET_REFERENCE())tsgl_framebuffer_setWithoutCheck, x, y, sprite, framebuffer->viewport_minX, framebuffer->viewport_minY, framebuffer->viewport_maxX, framebuffer->viewport_maxY);
}

inline void tsgl_framebuffer_line(tsgl_framebuffer* framebuffer, tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke) {
    tsgl_gfx_line(framebuffer, (TSGL_SET_REFERENCE())tsgl_framebuffer_setWithoutCheck, (TSGL_FILL_REFERENCE())tsgl_framebuffer_fill, x1, y1, x2, y2, color, stroke, framebuffer->viewport_minX, framebuffer->viewport_minY, framebuffer->viewport_maxX, framebuffer->viewport_maxY);
}

inline void tsgl_framebuffer_set(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
    if (!_pointInFrame(framebuffer, x, y)) return;
    tsgl_framebuffer_setWithoutCheck(framebuffer, x, y, color);
}

inline void tsgl_framebuffer_setWithoutCheck(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
    framebuffer->changed = true;
    tsgl_framebuffer_updateChangedAreaXY(framebuffer, x, y);

    size_t index;
    switch (framebuffer->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            index = _getRawBufferIndex(framebuffer, x, y);
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, index);
            _444write(index, framebuffer->buffer, color);
            break;

        case tsgl_monochrome:
            index = _getRawHorBufferIndex(framebuffer, x, y);
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, index);
            _monoWrite(index, _getHorOffset(framebuffer, x, y), framebuffer->buffer, color);
            break;

        case tsgl_rgb565_le:
        case tsgl_bgr565_le:
        case tsgl_rgb565_be:
        case tsgl_bgr565_be:
            index = _getBufferIndex(framebuffer, x, y);
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, index);
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, index + 1);
            _doubleSet(framebuffer, index, color);
            break;
        
        case tsgl_rgb888:
        case tsgl_bgr888:
            index = _getBufferIndex(framebuffer, x, y);
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, index);
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, index + 1);
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, index + 2);
            _doubleSet(framebuffer, index, color);
            framebuffer->buffer[index + 2] = color.arr[2];
            break;

        default: {
            index = _getBufferIndex(framebuffer, x, y);
            for (uint8_t i = 0; i < framebuffer->colorsize; i++) {
                tsgl_framebuffer_updateChangedAreaIndex(framebuffer, index + i);
                framebuffer->buffer[index + i] = color.arr[i];
            }
            break;
        }
    }
}

inline void tsgl_framebuffer_fill(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
    if (x < framebuffer->viewport_minX) {
        width = width - (framebuffer->viewport_minX - x);
        x = framebuffer->viewport_minX;
    }
    if (y < framebuffer->viewport_minY) {
        height = height - (framebuffer->viewport_minY - y);
        y = framebuffer->viewport_minY;
    }
    if (width + x > framebuffer->viewport_maxX) width = framebuffer->viewport_maxX - x;
    if (height + y > framebuffer->viewport_maxY) height = framebuffer->viewport_maxY - y;
    if (width <= 0 || height <= 0) return;
    if (x == 0 && y == 0 && width == framebuffer->width && height == framebuffer->height) {
        tsgl_framebuffer_clear(framebuffer, color);
        return;
    }
    return tsgl_framebuffer_fillWithoutCheck(framebuffer, x, y, width, height, color);
}

inline void tsgl_framebuffer_fillWithoutCheck(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
    tsgl_pos right = (x + width) - 1;
    tsgl_pos down = (y + height) - 1;
    framebuffer->changed = true;

    tsgl_framebuffer_updateChangedAreaXY(framebuffer, x, y);
    tsgl_framebuffer_updateChangedAreaXY(framebuffer, right, down);

    switch (framebuffer->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, _getRawBufferIndex(framebuffer, x, y));
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, _getRawBufferIndex(framebuffer, right, down));
            for (tsgl_pos ix = x; ix < x + width; ix++) {
                for (tsgl_pos iy = y; iy < y + height; iy++) {
                    _444write(_getRawBufferIndex(framebuffer, ix, iy), framebuffer->buffer, color);
                }
            }
            break;

        case tsgl_monochrome:
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, _getRawHorBufferIndex(framebuffer, x, y));
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, _getRawHorBufferIndex(framebuffer, right, down));
            for (tsgl_pos ix = x; ix < x + width; ix++) {
                for (tsgl_pos iy = y; iy < y + height; iy++) {
                    _monoWrite(_getRawHorBufferIndex(framebuffer, ix, iy), _getHorOffset(framebuffer, ix, iy), framebuffer->buffer, color);
                }
            }
            break;
        
        default:
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, _getBufferIndex(framebuffer, x, y));
            tsgl_framebuffer_updateChangedAreaIndex(framebuffer, _getBufferIndex(framebuffer, right, down) + (framebuffer->colorsize - 1));
            if (_isDenseColor(color, framebuffer->colorsize)) {
                switch (framebuffer->realRotation) {
                    case 1:
                        for (tsgl_pos ix = x; ix < x + width; ix++) {
                            memset(framebuffer->buffer + _getBufferIndex(framebuffer, ix, y + (height - 1)), color.arr[0], height * framebuffer->colorsize);
                        }
                        break;

                    case 2:
                        for (tsgl_pos iy = y; iy < y + height; iy++) {
                            memset(framebuffer->buffer + _getBufferIndex(framebuffer, x + (width - 1), iy), color.arr[0], width * framebuffer->colorsize);
                        }
                        break;

                    case 3:
                        for (tsgl_pos ix = x; ix < x + width; ix++) {
                            memset(framebuffer->buffer + _getBufferIndex(framebuffer, ix, y), color.arr[0], height * framebuffer->colorsize);
                        }
                        break;
                    
                    default:
                        for (tsgl_pos iy = y; iy < y + height; iy++) {
                            memset(framebuffer->buffer + _getBufferIndex(framebuffer, x, iy), color.arr[0], width * framebuffer->colorsize);
                        }
                        break;
                }
            } else {
                for (tsgl_pos iy = y; iy < y + height; iy++) {
                    for (tsgl_pos ix = x; ix < x + width; ix++) {
                        memcpy(framebuffer->buffer + _getBufferIndex(framebuffer, ix, iy), color.arr, framebuffer->colorsize);
                    }
                }
            }
            break;
    }
}

inline void tsgl_framebuffer_rect(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke) {
    tsgl_gfx_rect(framebuffer, (TSGL_FILL_REFERENCE())tsgl_framebuffer_fill, x, y, width, height, color, stroke);
}

inline tsgl_print_textArea tsgl_framebuffer_text(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text) {
    return tsgl_gfx_text(framebuffer, (TSGL_SET_REFERENCE())tsgl_framebuffer_setWithoutCheck, (TSGL_FILL_REFERENCE())tsgl_framebuffer_fillWithoutCheck, x, y, sets, text, framebuffer->viewport_minX, framebuffer->viewport_minY, framebuffer->viewport_maxX, framebuffer->viewport_maxY);
}

inline void tsgl_framebuffer_clear(tsgl_framebuffer* framebuffer, tsgl_rawcolor color) {
    if (framebuffer->viewport) {
        tsgl_framebuffer_fillWithoutCheck(framebuffer,
            framebuffer->viewport_minX,
            framebuffer->viewport_minY,
            framebuffer->viewport_maxX - framebuffer->viewport_minX,
            framebuffer->viewport_maxY - framebuffer->viewport_minY,
            color
        );
        return;
    }

    framebuffer->changed = true;
    tsgl_framebuffer_allChangedArea(framebuffer);

    switch (framebuffer->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            tsgl_color nonRawColor = tsgl_color_uraw(color, framebuffer->colormode);
            if (nonRawColor.r == nonRawColor.g && nonRawColor.g == nonRawColor.b) {
                memset(framebuffer->buffer, color.arr[0], framebuffer->buffersize);
            } else {
                size_t rawBuffersize = framebuffer->width * framebuffer->height;
                for (size_t i = 0; i <= rawBuffersize; i++) {
                    _444write(i, framebuffer->buffer, color);
                }
            }
            break;

        case tsgl_monochrome:
            if (color.arr[0]) {
                memset(framebuffer->buffer, 255, framebuffer->buffersize);
            } else {
                memset(framebuffer->buffer, 0, framebuffer->buffersize);
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

inline tsgl_rawcolor tsgl_framebuffer_getWithoutCheck(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    switch (framebuffer->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            return _444read(_getRawBufferIndex(framebuffer, x, y), framebuffer->buffer);
        
        default: {
            size_t index = _getBufferIndex(framebuffer, x, y);
            tsgl_rawcolor rawcolor;
            rawcolor.invalid = false;
            for (uint8_t i = 0; i < framebuffer->colorsize; i++) {
                rawcolor.arr[i] = framebuffer->buffer[index + i];
            }
            return rawcolor;
        }
    }
}

inline tsgl_rawcolor tsgl_framebuffer_get(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y) {
    if (!_pointInFrame(framebuffer, x, y)) return framebuffer->black;
    return tsgl_framebuffer_getWithoutCheck(framebuffer, x, y);
}

inline tsgl_rawcolor tsgl_framebuffer_rotationGet(tsgl_framebuffer* framebuffer, uint8_t rotation, tsgl_pos x, tsgl_pos y) {
    switch (framebuffer->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            return _444read(_rawRotateGetBufferIndex(framebuffer, rotation, x, y), framebuffer->buffer);
        
        default: {
            size_t index = _rotateGetBufferIndex(framebuffer, rotation, x, y);
            tsgl_rawcolor rawcolor;
            rawcolor.invalid = false;
            for (uint8_t i = 0; i < framebuffer->colorsize; i++) {
                rawcolor.arr[i] = framebuffer->buffer[index + i];
            }
            return rawcolor;
        }
    }
}