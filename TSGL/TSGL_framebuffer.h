#pragma once
#include "TSGL.h"
#include "TSGL_color.h"
#include "TSGL_font.h"
#include <esp_err.h>

typedef struct {
    uint8_t* buffer;
    size_t buffersize;
    tsgl_pos width;
    tsgl_pos height;
    tsgl_pos defaultWidth;
    tsgl_pos defaultHeight;
    tsgl_pos rotationWidth;
    float colorsize;
    bool floatColorsize;
    uint8_t rotation;
    uint8_t realRotation;
    tsgl_colormode colormode;
    tsgl_rawcolor black;
    bool hardwareRotate;
    bool softwareRotate;

    bool changed;
    int32_t changedFrom;
    int32_t changedTo;
    tsgl_pos changedUp;
    tsgl_pos changedDown;
    tsgl_pos changedLeft;
    tsgl_pos changedRight;

    bool viewport;
    tsgl_pos viewport_minX;
    tsgl_pos viewport_minY;
    tsgl_pos viewport_maxX;
    tsgl_pos viewport_maxY;

    bool dump_viewport;
    tsgl_pos dump_viewport_minX;
    tsgl_pos dump_viewport_minY;
    tsgl_pos dump_viewport_maxX;
    tsgl_pos dump_viewport_maxY;
} tsgl_framebuffer;

esp_err_t tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height, int64_t caps);
void tsgl_framebuffer_free(tsgl_framebuffer* framebuffer);

// changed area detector
void tsgl_framebuffer_resetChangedArea(tsgl_framebuffer* framebuffer);
void tsgl_framebuffer_allChangedArea(tsgl_framebuffer* framebuffer);
void tsgl_framebuffer_updateChangedAreaIndex(tsgl_framebuffer* framebuffer, int32_t index);
void tsgl_framebuffer_updateChangedAreaXY(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y);

// control
void tsgl_framebuffer_dumpViewport(tsgl_framebuffer* framebuffer);
void tsgl_framebuffer_flushViewport(tsgl_framebuffer* framebuffer);
void tsgl_framebuffer_clrViewport(tsgl_framebuffer* framebuffer);
void tsgl_framebuffer_setViewport(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height);

//these methods reset the viewport!
void tsgl_framebuffer_rotate(tsgl_framebuffer* framebuffer, uint8_t rotation); //rotates the indexing of the framebuffer and not the framebuffer itself
void tsgl_framebuffer_hardwareRotate(tsgl_framebuffer* framebuffer, uint8_t rotation); //it is assumed that this method will be used together with screen rotation via the tsgl_display_rotate method

// graphic
void tsgl_framebuffer_push(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_sprite* sprite); //allows you to draw sprites and rotate them when drawing. the tsgl_framebuffer_rotate method on sprite is ignored (because it changes the indexing)
void tsgl_framebuffer_setWithoutCheck(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color);
void tsgl_framebuffer_set(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color);
void tsgl_framebuffer_line(tsgl_framebuffer* framebuffer, tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke);
void tsgl_framebuffer_fill(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color);
void tsgl_framebuffer_fillWithoutCheck(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color);
void tsgl_framebuffer_rect(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke);
tsgl_print_textArea tsgl_framebuffer_text(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text);
void tsgl_framebuffer_clear(tsgl_framebuffer* framebuffer, tsgl_rawcolor color);
tsgl_rawcolor tsgl_framebuffer_getWithoutCheck(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y);
tsgl_rawcolor tsgl_framebuffer_get(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y);
tsgl_rawcolor tsgl_framebuffer_rotationGet(tsgl_framebuffer* framebuffer, uint8_t rotation, tsgl_pos x, tsgl_pos y);

#include "TSGL_gfx.h"