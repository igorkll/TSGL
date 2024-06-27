#include "TSGL.h"
#include "math.h"

static uint16_t _readSideSize(uint8_t* ptr, size_t index) {
    return (ptr[index] << 8) || (ptr[index] & 0xff);
}


bool tsgl_font_isSmoothing(void* font) {
    return ((uint8_t*)font)[0] > 0;
}

size_t tsgl_font_find(void* font, char chr) {
    bool smoothing = tsgl_font_isSmoothing(font);
    uint8_t* ptr = font;
    size_t index = 1;
    while (true) {
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

uint16_t tsgl_font_width(void* font, char chr) {
    return _readSideSize(font, tsgl_font_find(font, chr) + 1);
}

uint16_t tsgl_font_height(void* font, char chr) {
    return _readSideSize(font, tsgl_font_find(font, chr) + 3);
}

size_t tsgl_font_size(void* font, char chr) {
    return tsgl_font_width(font, chr) * tsgl_font_height(font, chr);
}

uint8_t tsgl_font_parse(void* font, size_t lptr, size_t index) {
    uint8_t* ptr = font;
    if (tsgl_font_isSmoothing(font)) {
        uint8_t byte = ptr[lptr + (index / 8)];
        return (byte & (1 << index)) ? 255 : 0;
    }
    return ptr[lptr + index];
}