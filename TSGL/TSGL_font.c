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

size_t tsgl_font_len(const char* str) { //custom strlen
    size_t size = 0;
    while (str[size] != '\n' && str[size] != '\0') size++;
    return size;
}

tsgl_print_textArea tsgl_font_rasterize(void* arg, TSGL_SET_REFERENCE(set), TSGL_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_pos screenWidth, tsgl_pos screenHeight, tsgl_print_settings sets, const char* text) {
    size_t realsize = strlen(text);
    tsgl_print_textArea textArea = {
        .strlen = realsize
    };

    if (sets._scaleX == 0) sets._scaleX = 1;
    if (sets._scaleY == 0) sets._scaleY = 1;
    if (sets.scaleX == 0) sets.scaleX = 1;
    if (sets.scaleY == 0) sets.scaleY = 1;

    tsgl_pos standartWidth = tsgl_font_width(sets.font, 'A');
    if (sets.targetWidth != 0) {
        sets._scaleX = ((float)sets.targetWidth) / ((float)standartWidth);
    }
    standartWidth = (((float)standartWidth) * sets._scaleX) + 0.5;

    tsgl_pos standartHeight = tsgl_font_height(sets.font, 'A');
    if (sets.targetHeight == 0) {
        sets.targetHeight = sets.targetWidth;
    }
    if (sets.targetHeight != 0) {
        sets._scaleY = ((float)sets.targetHeight) / ((float)standartHeight);
    }
    standartHeight = (((float)standartHeight) * sets._scaleY) + 0.5;

    tsgl_pos spacing = sets.spacing > 0 ? sets.spacing : (standartWidth / 4);
    if (spacing <= 0) spacing = 1;

    if (!sets.fill.invalid && fill != NULL) {
        tsgl_print_textArea textArea = tsgl_font_getTextArea(x, y, screenWidth, screenHeight, sets, text);
        fill(arg, textArea.left, textArea.top, textArea.width, textArea.height, sets.fill);
    }

    if (sets.multiline) {
        tsgl_pos oldX = x;

        if (sets.globalCentering) {
            tsgl_print_settings lSets;
            memcpy(&lSets, &sets, sizeof(tsgl_print_settings));
            lSets.globalCentering = false;
            lSets._minWidth = oldX;
            lSets._maxWidth = (oldX + sets.width) - 1;
            lSets._clamp = true;

            switch (sets.locationMode) {
                case tsgl_print_start_bottom:
                    lSets._minHeight = (y - sets.height) + 1;
                    lSets._maxHeight = y;
                    break;

                case tsgl_print_start_top:
                    lSets._minHeight = y;
                    lSets._maxHeight = (y + sets.height) - 1;
                    break;
            }

            tsgl_print_textArea textArea = tsgl_font_getTextArea(x, y, screenWidth, screenHeight, lSets, text);
            if (sets.width > 0) x = (x + (sets.width / 2)) - (textArea.width / 2);
            if (sets.height > 0) y = (y + (sets.height / 2)) - (textArea.height / 2);
        }

        tsgl_print_settings newSets = {
            .font = sets.font,
            .fill = TSGL_INVALID_RAWCOLOR,
            .bg = sets.bg,
            .fg = sets.fg,
            ._minWidth = oldX,
            ._maxWidth = (oldX + sets.width) - 1,
            ._clamp = true,
            .scaleX = sets.scaleX,
            .scaleY = sets.scaleY,
            ._scaleX = sets._scaleX,
            ._scaleY = sets._scaleY,
            .spacing = sets.spacing,
            .spaceSize = sets.spaceSize,
            .locationMode = sets.locationMode
        };

        switch (sets.locationMode) {
            case tsgl_print_start_bottom:
                newSets._minHeight = (y - sets.height) + 1;
                newSets._maxHeight = y;
                break;

            case tsgl_print_start_top:
                newSets._minHeight = y;
                newSets._maxHeight = (y + sets.height) - 1;
                break;
        }

        textArea.top = TSGL_POS_MAX;
        textArea.bottom = TSGL_POS_MIN;
        textArea.left = TSGL_POS_MAX;
        textArea.right = TSGL_POS_MIN;

        tsgl_pos currentY = y;
        for (size_t i = 0; i < realsize;) {
            tsgl_print_textArea lTextArea = tsgl_font_rasterize(arg, set, fill, x, currentY, screenWidth, screenHeight, newSets, text + i);
            if (lTextArea.top < textArea.top) textArea.top = lTextArea.top;
            if (lTextArea.bottom > textArea.bottom) textArea.bottom = lTextArea.bottom;
            if (lTextArea.left < textArea.left) textArea.left = lTextArea.left;
            if (lTextArea.right > textArea.right) textArea.right = lTextArea.right;
            i += lTextArea.strlen + 1;
            switch (sets.locationMode) {
                case tsgl_print_start_bottom:
                    currentY -= lTextArea.height + spacing;
                    break;

                case tsgl_print_start_top:
                    currentY += lTextArea.height + spacing;
                    break;
            }
        }
        textArea.width = (textArea.right - textArea.left) + 1;
        textArea.height = (textArea.bottom - textArea.top) + 1;
        return textArea;
    }

    textArea.left = x;
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
    size_t strsize = tsgl_font_len(text);
    textArea.strlen = strsize;
    tsgl_pos offset = 0;
    for (size_t i = 0; i < strsize; i++) {
        char chr = text[i];
        if (chr != ' ') {
            size_t charPosition = tsgl_font_find(sets.font, chr);
            if (charPosition > 0) {
                uint16_t charWidth = tsgl_font_width(sets.font, chr);
                uint16_t charHeight = tsgl_font_height(sets.font, chr);
                uint16_t scaleCharWidth = ((float)charWidth * sets._scaleX * sets.scaleX) + 0.5;
                uint16_t scaleCharHeight = ((float)charHeight * sets._scaleY * sets.scaleY) + 0.5;
                for (tsgl_pos iy = 0; iy < scaleCharHeight; iy++) {
                    tsgl_pos checkPy = 0;
                    switch (sets.locationMode) {
                        case tsgl_print_start_bottom:
                            checkPy = y - iy;
                            break;
                        case tsgl_print_start_top:
                            checkPy = y + iy;
                            break;
                    }
                    if (checkPy < 0) break;
                    if (screenHeight > 0 && checkPy >= screenHeight) break;
                    if (sets._clamp) {
                        if (checkPy < sets._minHeight) continue;
                        if (checkPy > sets._maxHeight) break;
                    }

                    for (tsgl_pos ix = 0; ix < scaleCharWidth; ix++) {
                        tsgl_pos px = x + ix + offset;
                        if (px < 0) break;
                        if (screenWidth > 0 && px >= screenWidth) break;
                        if (sets._clamp) {
                            if (px < sets._minWidth) continue;
                            if (px > sets._maxWidth) break;
                        }

                        tsgl_pos oix = ((float)ix) / sets._scaleX / sets.scaleX;
                        tsgl_pos oiy = ((float)iy) / sets._scaleY / sets.scaleY;

                        tsgl_pos py = 0;
                        size_t index = 0;
                        switch (sets.locationMode) {
                            case tsgl_print_start_bottom:
                                index = oix + (((charHeight - 1) - oiy) * charWidth);
                                py = y - iy;
                                if (py < textArea.top) textArea.top = py;
                                break;
                            case tsgl_print_start_top:
                                index = oix + (oiy * charWidth);
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
                offset += scaleCharWidth + spacing;
            }
        } else {
            tsgl_pos spaceSize;
            if (sets.spaceSize == 0) {
                spaceSize = standartWidth * 0.7;
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

tsgl_print_textArea tsgl_font_getTextArea(tsgl_pos x, tsgl_pos y, tsgl_pos screenWidth, tsgl_pos screenHeight, tsgl_print_settings sets, const char* text) {
    return tsgl_font_rasterize(NULL, NULL, NULL, x, y, screenWidth, screenHeight, sets, text);
}