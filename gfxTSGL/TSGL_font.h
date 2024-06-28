#pragma once
#include "TSGL.h"
#include "TSGL_color.h"

typedef enum {
    tsgl_print_start_bottom = 0, //the text is drawn starting from the bottom left point
    tsgl_print_start_top //starting from the top left point
} tsgl_print_locationMode;

typedef struct {
    const void* font;
    tsgl_rawcolor bg;
    tsgl_rawcolor fg;

    float scale; //if 0, scaling is disabled
    tsgl_pos spacing; //the distance between characters. if 0, is calculated automatically
    tsgl_pos spaceSize; //the size of the space character. if 0, is calculated automatically
    tsgl_print_locationMode locationMode;
} tsgl_print_settings;

bool tsgl_font_isSmoothing(const void* font);
size_t tsgl_font_find(const void* font, char chr);
uint16_t tsgl_font_width(const void* font, char chr);
uint16_t tsgl_font_height(const void* font, char chr);
uint8_t tsgl_font_parse(const void* font, size_t lptr, size_t index);

// this function allows you to calculate in advance in which area the text will be drawn
typedef struct {
    tsgl_pos left;
    tsgl_pos top;
    tsgl_pos width;
    tsgl_pos height;
} tsgl_print_textArea;

tsgl_print_textArea tsgl_font_getTextArea(tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text);