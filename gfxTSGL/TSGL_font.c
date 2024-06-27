#include "TSGL.h"
#include "math.h"
#include <stdio.h>

static uint16_t _readSideSize(const uint8_t* ptr, size_t index) {
    return (ptr[index] << 8) | (ptr[index + 1] & 0xff);
}


bool tsgl_font_isSmoothing(const void* font) {
    return ((const uint8_t*)font)[0] > 0;
}

size_t tsgl_font_find(const void* font, char chr) {
    bool smoothing = tsgl_font_isSmoothing(font);
    const uint8_t* ptr = font;
    size_t index = 1;
    while (true) {
        printf("tsgl_font_find: %i %c\n", index, ptr[index]);
        if (ptr[index] == chr) break;
        if (ptr[index] == 0) return 0;
        size_t charSize = _readSideSize(font, index + 1) * _readSideSize(font, index + 3);
        if (!smoothing) {
            charSize = ceil(charSize / 8.0);
        }
        index = charSize + 5;
    }
    return index + 5;
}

uint16_t tsgl_font_width(const void* font, char chr) {
    size_t index = tsgl_font_find(font, chr);
    if (index == 0) return 0;
    return _readSideSize(font, index - 4);
}

uint16_t tsgl_font_height(const void* font, char chr) {
    size_t index = tsgl_font_find(font, chr);
    if (index == 0) return 0;
    return _readSideSize(font, index - 2);
}

uint8_t tsgl_font_parse(const void* font, size_t lptr, size_t index) {
    const uint8_t* ptr = font;
    if (!tsgl_font_isSmoothing(font)) {
        uint8_t byte = ptr[lptr + (index / 8)];
        return (byte & (1 << index)) ? 255 : 0;
    }
    return ptr[lptr + index];
}