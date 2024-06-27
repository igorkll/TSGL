#pragma once
#include "TSGL.h"
#include "TSGL_color.h"

typedef struct {
    const void* font;
    float scale; //if 0, scaling is disabled
    tsgl_rawcolor bg;
    tsgl_rawcolor fg;
    tsgl_pos spacing; //the distance between characters. if 0, is calculated automatically
    tsgl_pos spaceSize; //the size of the space character. if 0, is calculated automatically
} tsgl_print_settings;

bool tsgl_font_isSmoothing(const void* font);
size_t tsgl_font_find(const void* font, char chr);
size_t tsgl_font_width(const void* font, char chr);
size_t tsgl_font_height(const void* font, char chr);
uint8_t tsgl_font_parse(const void* font, size_t lptr, size_t index);