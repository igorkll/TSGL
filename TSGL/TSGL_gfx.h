#pragma once
#include "TSGL.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"

struct tsgl_sprite { //the type is declared in "TSGL.h"
    uint8_t rotation;
    tsgl_framebuffer* sprite;
    tsgl_rawcolor transparentColor;
    bool flixX;
    bool flixY;
};

void tsgl_gfx_rect(void* arg, TSGL_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke);
void tsgl_gfx_line(void* arg, TSGL_SET_REFERENCE(set), TSGL_FILL_REFERENCE(fill), tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke, tsgl_pos screenWidth, tsgl_pos screenHeight);
void tsgl_gfx_push(void* arg, TSGL_SET_REFERENCE(set), tsgl_pos x, tsgl_pos y, tsgl_sprite* sprite, tsgl_pos screenWidth, tsgl_pos screenHeight);