#pragma once
#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"

typedef enum {
    tsgl_gui_absolute,
    tsgl_gui_percent
} tsgl_gui_paramFormat;

typedef struct tsgl_gui tsgl_gui;
struct tsgl_gui {
    // settings
    tsgl_gui_paramFormat format_x; float x;
    tsgl_gui_paramFormat format_y; float y;
    tsgl_gui_paramFormat format_width; float width;
    tsgl_gui_paramFormat format_height; float height;

    bool interactive;
    bool displayable;

    // internal
    void* target;
    bool buffered;
    tsgl_colormode colormode;

    tsgl_pos math_x;
    tsgl_pos math_y;
    tsgl_pos math_width;
    tsgl_pos math_height;

    size_t parentsCount;
    tsgl_gui** parents;
    tsgl_gui* parent;

    void* data;
};

tsgl_gui* tsgl_gui_createRoot_display(tsgl_display* display, tsgl_colormode colormode); //you specify the colormode yourself because on some screens it must be different from the colormode specified in the driver
tsgl_gui* tsgl_gui_createRoot_buffer(tsgl_framebuffer* framebuffer);
tsgl_gui* tsgl_gui_addObject(tsgl_gui* object);
void tsgl_gui_free(tsgl_gui* object);

void tsgl_gui_math(tsgl_gui* root);
void tsgl_gui_draw(tsgl_gui* root);