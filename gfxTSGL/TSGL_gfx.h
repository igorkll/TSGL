#pragma once
#include "TSGL_gfx.h"
#include "TSGL_color.h"

#define TSGL_GFX_SET_REFERENCE(name) void(*name)(void* arg, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color)
#define TSGL_GFX_FILL_REFERENCE(name) void(*name)(void* arg, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color)

void tsgl_gfx_rect(void* arg, TSGL_GFX_FILL_REFERENCE(fill), tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke);
void tsgl_gfx_line(void* arg, TSGL_GFX_SET_REFERENCE(set), TSGL_GFX_FILL_REFERENCE(fill), tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke);