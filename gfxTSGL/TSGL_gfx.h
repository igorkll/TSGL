#pragma once
#include "TSGL_gfx.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include "TSGL_font.h"

#define TSGL_GFX_SET_REFERENCE(name) void(*name)(void* arg, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color)
#define TSGL_GFX_FILL_REFERENCE(name) void(*name)(void* arg, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color)

void tsgl_gfx_rect(void* arg, TSGL_GFX_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke);
void tsgl_gfx_line(void* arg, TSGL_GFX_SET_REFERENCE(set), TSGL_GFX_FILL_REFERENCE(fill), tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke, tsgl_pos screenWidth, tsgl_pos screenHeight);
void tsgl_gfx_push(void* arg, TSGL_GFX_SET_REFERENCE(set), tsgl_pos x, tsgl_pos y, uint8_t rotation, tsgl_framebuffer* sprite, tsgl_rawcolor transparentColor, tsgl_pos screenWidth, tsgl_pos screenHeight);
//you can make the background or text transparent using TSGL_INVALID_RAWCOLOR
void tsgl_gfx_text(void* arg, TSGL_GFX_SET_REFERENCE(set), tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text);