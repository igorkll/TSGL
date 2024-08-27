#include "TSGL.h"
#include "TSGL_gfx.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include "TSGL_font.h"
#include "TSGL_math.h"
#include <ESP_LOG.h>
#include <string.h>

static const char* TAG = "TSGL_gfx";

void tsgl_gfx_rect(void* arg, TSGL_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke) {
    fill(arg, x, y, width, stroke, color);
    fill(arg, x, (y + height) - stroke, width, stroke, color);
    fill(arg, x, y + 1, stroke, height - 2, color);
    fill(arg, (x + width) - stroke, y + 1, stroke, height - 2, color);
}

void tsgl_gfx_line(void* arg, TSGL_SET_REFERENCE(set), TSGL_FILL_REFERENCE(fill), tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke, tsgl_pos screenWidth, tsgl_pos screenHeight) {
    if (x1 < 0) x1 = 0; else if (x1 >= screenWidth) x1 = screenWidth - 1;
    if (y1 < 0) y1 = 0; else if (y1 >= screenHeight) y1 = screenHeight - 1;
    if (x2 < 0) x2 = 0; else if (x2 >= screenWidth) x2 = screenWidth - 1;
    if (y2 < 0) y2 = 0; else if (y2 >= screenHeight) y2 = screenHeight - 1;
    tsgl_pos strokeD = stroke / 2;
    tsgl_pos fx = (x1 > x2 ? x2 : x1) - (stroke / 2);
    tsgl_pos fy = (y1 > y2 ? y2 : y1) - (stroke / 2);
    if (y1 == y2) {
        fill(arg, fx, fy, abs(x2 - x1), stroke, color);
        return;
    } else if (x1 == x2) {
        fill(arg, fx, fy, stroke, abs(y2 - y1), color);
        return;
    }
    tsgl_pos inLoopValueFrom;
    tsgl_pos inLoopValueTo;
    tsgl_pos outLoopValueFrom;
    tsgl_pos outLoopValueTo;
    bool isReversed;
    tsgl_pos inLoopValueDelta = abs(x2 - x1);
    tsgl_pos outLoopValueDelta = abs(y2 - y1);
    if (inLoopValueDelta < outLoopValueDelta) {
        tsgl_pos t = inLoopValueDelta;
        inLoopValueDelta = outLoopValueDelta;
        outLoopValueDelta = t;

        inLoopValueFrom = y1;
        inLoopValueTo = y2;
        outLoopValueFrom = x1;
        outLoopValueTo = x2;
        isReversed = true;
    } else {
        inLoopValueFrom = x1;
        inLoopValueTo = x2;
        outLoopValueFrom = y1;
        outLoopValueTo = y2;
        isReversed = false;
    }

    if (outLoopValueFrom > outLoopValueTo) {
        tsgl_pos t = inLoopValueFrom;
        inLoopValueFrom = inLoopValueTo;
        inLoopValueTo = t;

        t = outLoopValueFrom;
        outLoopValueFrom = outLoopValueTo;
        outLoopValueTo = t;
    }

    tsgl_pos outLoopValue = outLoopValueFrom;
    tsgl_pos outLoopValueCounter = 1;
    float outLoopValueTriggerIncrement = (float)inLoopValueDelta / (float)outLoopValueDelta;
    float outLoopValueTrigger = outLoopValueTriggerIncrement;

    for (tsgl_pos inLoopValue = inLoopValueFrom; inLoopValue <= inLoopValueTo; inLoopValue += (inLoopValueFrom < inLoopValueTo ? 1 : -1)) {
        if (stroke > 1) {
            if (isReversed) {
                fill(arg, outLoopValue - strokeD, inLoopValue - strokeD, stroke, stroke, color);
            } else {
                fill(arg, inLoopValue - strokeD, outLoopValue - strokeD, stroke, stroke, color);
            }
        } else {
            if (isReversed) {
                set(arg, outLoopValue, inLoopValue, color);
            } else {
                set(arg, inLoopValue, outLoopValue, color);
            }
        }

        outLoopValueCounter++;
        if (outLoopValueCounter > outLoopValueTrigger) {
            outLoopValue++;
            outLoopValueTrigger += outLoopValueTriggerIncrement;
        }
    }
}

void tsgl_gfx_push(void* arg, TSGL_SET_REFERENCE(set), tsgl_pos x, tsgl_pos y, tsgl_sprite* sprite, tsgl_pos screenWidth, tsgl_pos screenHeight) {
    sprite->rotation = ((uint8_t)(-sprite->rotation)) % (uint8_t)4;

    if (sprite->sprite->hardwareRotate) {
        ESP_LOGE(TAG, "a sprite cannot have a hardware rotation");
        return;
    }

    tsgl_pos realSpriteWidth;
    tsgl_pos realSpriteHeight;
    switch (sprite->rotation) {
        case 1:
        case 3:
            realSpriteWidth = sprite->sprite->defaultHeight;
            realSpriteHeight = sprite->sprite->defaultWidth;
            break;

        default:
            realSpriteWidth = sprite->sprite->defaultWidth;
            realSpriteHeight = sprite->sprite->defaultHeight;
            break;
    }

    tsgl_pos spriteWidth = realSpriteWidth;
    if (sprite->resizeWidth != 0) spriteWidth = sprite->resizeWidth;
    tsgl_pos spriteHeight = realSpriteHeight;
    if (sprite->resizeHeight != 0) spriteHeight = sprite->resizeHeight;

    tsgl_pos startX = 0;
    tsgl_pos startY = 0;
    if (x < 0) startX = -x;
    if (y < 0) startY = -y;
    tsgl_pos maxSpriteWidth = screenWidth - x;
    tsgl_pos maxSpriteHeight = screenHeight - y;
    tsgl_pos spriteMaxPointX = spriteWidth - 1;
    tsgl_pos spriteMaxPointY = spriteHeight - 1;
    tsgl_pos spriteRealMaxPointX = realSpriteWidth - 1;
    tsgl_pos spriteRealMaxPointY = realSpriteHeight - 1;
    if (spriteWidth > maxSpriteWidth) spriteWidth = maxSpriteWidth;
    if (spriteHeight > maxSpriteHeight) spriteHeight = maxSpriteHeight;
    for (tsgl_pos posX = startX; posX < spriteWidth; posX++) {
        tsgl_pos setPosX = posX + x;
        for (tsgl_pos posY = startY; posY < spriteHeight; posY++) {
            tsgl_pos setPosY = posY + y;
            tsgl_pos getPosX = sprite->flixX ? (spriteMaxPointX - posX) : posX;
            tsgl_pos getPosY = sprite->flixY ? (spriteMaxPointY - posY) : posY;
            tsgl_rawcolor color = tsgl_framebuffer_rotationGet(sprite->sprite, sprite->rotation,
                sprite->resizeWidth == 0 ? getPosX : tsgl_math_imap(getPosX, 0, spriteMaxPointX, 0, spriteRealMaxPointX),
                sprite->resizeHeight == 0 ? getPosY : tsgl_math_imap(getPosY, 0, spriteMaxPointY, 0, spriteRealMaxPointY)
            );

            if (sprite->transparentColor.invalid || !tsgl_color_rawColorCompare(color, sprite->transparentColor, sprite->sprite->colorsize)) {
                set(arg, setPosX, setPosY, color);
            }
        }
    }
}

tsgl_print_textArea tsgl_gfx_text(void* arg, TSGL_SET_REFERENCE(set), TSGL_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text, tsgl_pos screenWidth, tsgl_pos screenHeight) {
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
            tsgl_print_textArea lTextArea = tsgl_gfx_text(arg, set, fill, x, currentY, newSets, text + i, screenWidth, screenHeight);
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
                    if (screenHeight > 0 && checkPy >= screenHeight) break;
                    if (sets._clamp) {
                        if (checkPy < sets._minHeight) continue;
                        if (checkPy > sets._maxHeight) break;
                    }

                    for (tsgl_pos ix = 0; ix < scaleCharWidth; ix++) {
                        tsgl_pos px = x + ix + offset;
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
                        if (set != NULL && px >= 0 && py >= 0) {
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