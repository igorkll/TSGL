#pragma once
#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"

typedef struct tsgl_gui tsgl_gui;
struct tsgl_gui {
    // settings
    tsgl_pos x;
    tsgl_pos y;
    tsgl_pos width;
    tsgl_pos height;
    bool interactive;
    bool displayable;

    // internal
    void* target;
    bool buffered;

    tsgl_pos math_x;
    tsgl_pos math_y;
    tsgl_pos math_width;
    tsgl_pos math_height;

    size_t parentsCount;
    tsgl_gui** parents;
    tsgl_gui* parent;

    void* data;
};

tsgl_gui* tsgl_gui_createRoot_display(tsgl_display* display);
tsgl_gui* tsgl_gui_createRoot_buffer(tsgl_framebuffer* framebuffer);
tsgl_gui* tsgl_gui_addObject(tsgl_gui* object);
void tsgl_gui_free(tsgl_gui* object);

void tsgl_gui_math(tsgl_gui* object);
void tsgl_gui_draw(tsgl_gui* object);