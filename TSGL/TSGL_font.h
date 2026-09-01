#pragma once
#include "TSGL.h"
#include "TSGL_color.h"
#include "TSGL_fonts/default.h"

typedef enum {
    tsgl_print_start_bottom = 0, //the text is drawn starting from the bottom left point
    tsgl_print_start_top //starting from the top left point
} tsgl_print_locationMode;

typedef enum {
    tsgl_print_localLocationMode_from_localtionMode = 0,
    tsgl_print_localLocationMode_bottom,
    tsgl_print_localLocationMode_top,
    tsgl_print_localLocationMode_center
} tsgl_print_localLocationMode;

typedef enum {
    tsgl_print_alignment_left = 0,
    tsgl_print_alignment_center,
    tsgl_print_alignment_right,
} tsgl_print_alignment;

typedef struct {
    const void* font;
    tsgl_rawcolor fill;
    tsgl_rawcolor bg; //you can make the background or text transparent using TSGL_INVALID_RAWCOLOR
    tsgl_rawcolor fg;

    float scaleX; //if 0, scaling is disabled
    float scaleY;
    tsgl_pos targetWidth;
    tsgl_pos targetHeight;

    tsgl_pos spacing; //the distance between characters. if 0, is calculated automatically
    tsgl_pos spaceSize; //the size of the space character. if 0, is calculated automatically

    // note that if you set the "tsgl_print_start_bottom" mode, the text will be drawn above the call point,
    // which also means that the lines will be inverted during multiline output and the "linesRevers" flag must be set so that they go in the correct order
    // By default, it also affects where each individual letter will be drawn from. to unlink this parameter, set "localLocationMode"
    // often the best solution is to set "locationMode" to "tsgl_print_start_top" and if you need the text to be at the bottom of the block, then set "globalAlignmentY" to "tsgl_print_alignment_right"
    tsgl_print_locationMode locationMode;

    // default: tsgl_print_localLocationMode_from_localtionMode which means binding to the "locationMode" parameter
    // outputs where each individual letter will be output from
    tsgl_print_localLocationMode localLocationMode;

    // the following options only work if multiline is enabled
    bool multiline;
    bool linesRevers; // CURRENTLY NOT IMPLEMENTED
    tsgl_pos width; //the width and height can be specified so that the text does not bulge out of the frame
    tsgl_pos height;
    bool globalCentering; //place the content in the middle of the box. similar to installing globalAlignmentX and globalAlignmentY on tsgl_print_alignment_center
    tsgl_print_alignment globalAlignmentX;
    tsgl_print_alignment globalAlignmentY;
    tsgl_print_alignment alignment;
    
    tsgl_pos _minWidth;
    tsgl_pos _maxWidth;
    tsgl_pos _minHeight;
    tsgl_pos _maxHeight;
    bool _clamp;
    float _scaleX;
    float _scaleY;
} tsgl_print_settings;

typedef struct {
    tsgl_pos left;
    tsgl_pos top;
    tsgl_pos right;
    tsgl_pos bottom;
    tsgl_pos width;
    tsgl_pos height;
    size_t strlen;
} tsgl_print_textArea;

size_t tsgl_font_find(const void* font, char chr);
uint16_t tsgl_font_width(const void* font, char chr);
uint16_t tsgl_font_height(const void* font, char chr);
bool tsgl_font_parse(const void* font, size_t lptr, size_t index);
tsgl_print_textArea tsgl_font_getTextArea(tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text); // this function allows you to calculate in advance in which area the text will be drawn