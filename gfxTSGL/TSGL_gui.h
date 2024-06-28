#pragma once
#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"

typedef struct {
    tsgl_pos x;
    tsgl_pos y;
    tsgl_pos width;
    tsgl_pos height;
    tsgl_rawcolor color;
    tsgl_gui* gui;
} tsgl_gui_container;

typedef struct {
    tsgl_gui_container* containers;
    void* target;
    bool buffered;
} tsgl_gui;

tsgl_gui* tsgl_gui_createForDisplay(tsgl_display* display);
tsgl_gui* tsgl_gui_createForBuffer(tsgl_framebuffer* framebuffer);
tsgl_gui_container* tsgl_gui_addContainer(tsgl_gui* gui, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color);
void tsgl_gui_freeContainer(tsgl_gui_container* container);
void tsgl_gui_free(tsgl_gui* gui);