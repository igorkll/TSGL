#include "TSGL.h"
#include "TSGL_font.h"
#include "TSGL_gfx.h"
#include <math.h>
#include <string.h>

static uint16_t _read_uint16(const uint8_t* ptr, size_t index) {
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
        if (ptr[index] == chr) break;
        if (ptr[index] == 0) return 0;
        size_t charSize = _read_uint16(font, index + 1) * _read_uint16(font, index + 3);
        if (!smoothing) charSize = ceil(charSize / 8.0);
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
    if (!tsgl_font_isSmoothing(font)) {
        uint8_t byte = ptr[lptr + (index / 8)];
        return byte & (1 << (index % 8)) ? 255 : 0;
    }
    return ptr[lptr + index];
}

tsgl_print_textArea tsgl_font_rasterize(void* arg, TSGL_SET_REFERENCE(set), tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text) {
    tsgl_print_textArea textArea = {
        .left = x
    };
    switch (sets.locationMode) {
        case tsgl_print_start_bottom:
            textArea.top = TSGL_POS_MAX;
            textArea.bottom = y;
            break;
        case tsgl_print_start_top:
            textArea.top = y;
            textArea.bottom = TSGL_POS_MIN;
            break;
    }
    size_t strsize = strlen(text);
    tsgl_pos offset = 0;
    tsgl_pos standartWidth = tsgl_font_width(sets.font, 'A');
    tsgl_pos spacing = sets.spacing > 0 ? sets.spacing : (standartWidth / 4);
    if (spacing <= 0) spacing = 1;
    for (size_t i = 0; i < strsize; i++) {
        char chr = text[i];
        if (chr != ' ') {
            size_t charPosition = tsgl_font_find(sets.font, chr);
            if (charPosition > 0) {
                uint16_t charWidth = tsgl_font_width(sets.font, chr);
                uint16_t charHeight = tsgl_font_height(sets.font, chr);
                for (tsgl_pos iy = 0; iy < charWidth; iy++) {
                    for (tsgl_pos ix = 0; ix < charHeight; ix++) {
                        tsgl_pos px = x + ix + offset;
                        tsgl_pos py = 0;
                        size_t index = 0;
                        switch (sets.locationMode) {
                            case tsgl_print_start_bottom:
                                index = ix + (((charHeight - 1) - iy) * charWidth);
                                py = y - iy;
                                if (py < textArea.top) textArea.top = py;
                                break;
                            case tsgl_print_start_top:
                                index = ix + (iy * charWidth);
                                py = y + iy;
                                if (py > textArea.bottom) textArea.bottom = py;
                                break;
                        }
                        if (px > textArea.right) textArea.right = px;
                        if (set != NULL) {
                            uint8_t value = tsgl_font_parse(sets.font, charPosition, index);
                            if (value == 255) {
                                if (!sets.fg.invalid) set(arg, px, py, sets.fg);
                            } else if (value == 0) {
                                if (!sets.bg.invalid) set(arg, px, py, sets.bg);
                            }
                        }
                    }
                }
                offset += charWidth + spacing;
            }
        } else {
            tsgl_pos spaceSize;
            if (sets.spaceSize == 0) {
                spaceSize = standartWidth;
            } else {
                spaceSize = sets.spaceSize;
            }
            tsgl_pos staceEndPos = x + spaceSize + offset;
            if (staceEndPos > textArea.right) textArea.right = staceEndPos;
            offset += spaceSize + spacing;
        }
    }
    textArea.width = (textArea.right - textArea.left) + 1;
    textArea.height = (textArea.bottom - textArea.top) + 1;
    return textArea;
}

tsgl_print_textArea tsgl_font_getTextArea(tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text) {
    return tsgl_font_rasterize(NULL, NULL, x, y, sets, text);
}