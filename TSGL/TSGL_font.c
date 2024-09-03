#include "TSGL.h"
#include "TSGL_font.h"
#include "TSGL_gfx.h"
#include <math.h>
#include <string.h>

static uint16_t _read_uint16(const uint8_t* ptr, size_t index) {
    return (ptr[index] << 8) | (ptr[index + 1] & 0xff);
}

static uint8_t _version(const void* font) {
    return ((const uint8_t*)font)[0];
}



size_t tsgl_font_find(const void* font, char chr) {
    const uint8_t* ptr = font;
    size_t index = 1;
    while (true) {
        if (ptr[index] == chr) break;
        if (ptr[index] == 0) return 0;
        size_t charSize = ceil((_read_uint16(font, index + 1) * _read_uint16(font, index + 3)) / 8.0);
        index += charSize + 5;
    }
    return index + 5;
}

uint16_t tsgl_font_width(const void* font, char chr) {
    size_t index = tsgl_font_find(font, chr);
    if (index == 0) return 0;
    return _read_uint16(font, index - 4);
}

uint16_t tsgl_font_height(const void* font, char chr) {
    size_t index = tsgl_font_find(font, chr);
    if (index == 0) return 0;
    return _read_uint16(font, index - 2);
}

uint8_t tsgl_font_parse(const void* font, size_t lptr, size_t index) {
    const uint8_t* ptr = font;
    uint8_t byte = ptr[lptr + (index / 8)];
    return byte & (1 << (index % 8)) ? 255 : 0;
    return ptr[lptr + index];
}

size_t tsgl_font_len(const char* str) { //custom strlen
    size_t size = 0;
    while (str[size] != '\n' && str[size] != '\0') size++;
    return size;
}

tsgl_print_textArea tsgl_font_getTextArea(tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text) {
    return tsgl_gfx_text(NULL, NULL, NULL, x, y, sets, text, TSGL_POS_MIN, TSGL_POS_MIN, TSGL_POS_MAX, TSGL_POS_MAX);
}