#include "TSGL.h"
#include "TSGL_gfx.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include "TSGL_font.h"
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

    tsgl_pos spriteWidth;
    tsgl_pos spriteHeight;
    switch (sprite->rotation) {
        case 1:
        case 3:
            spriteWidth = sprite->sprite->defaultHeight;
            spriteHeight = sprite->sprite->defaultWidth;
            break;

        default:
            spriteWidth = sprite->sprite->defaultWidth;
            spriteHeight = sprite->sprite->defaultHeight;
            break;
    }

    tsgl_pos startX = 0;
    tsgl_pos startY = 0;
    if (x < 0) startX = -x;
    if (y < 0) startY = -y;
    tsgl_pos maxSpriteWidth = screenWidth - x;
    tsgl_pos maxSpriteHeight = screenHeight - y;
    if (spriteWidth > maxSpriteWidth) spriteWidth = maxSpriteWidth;
    if (spriteHeight > maxSpriteHeight) spriteHeight = maxSpriteHeight;
    tsgl_pos spriteMaxPointX = spriteWidth - 1;
    tsgl_pos spriteMaxPointY = spriteHeight - 1;
    for (tsgl_pos posX = startX; posX < spriteWidth; posX++) {
        tsgl_pos lPosX = posX;
        if (sprite->flixX) lPosX = spriteMaxPointX - lPosX;
        tsgl_pos setPosX = lPosX + x;
        for (tsgl_pos posY = startY; posY < spriteHeight; posY++) {
            tsgl_pos lPosY = posY;
            if (sprite->flixY) lPosY = spriteMaxPointY - lPosY;
            tsgl_pos setPosY = lPosY + y;
            tsgl_rawcolor color = tsgl_framebuffer_rotationGet(sprite->sprite, sprite->rotation, posX, posY);
            if (sprite->transparentColor.invalid || memcmp(color.arr, sprite->transparentColor.arr, sprite->sprite->colorsize) != 0) {
                set(arg, setPosX, setPosY, color);
            }
        } 
    }
}