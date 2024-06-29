#pragma once
#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"

typedef struct tsgl_gui tsgl_gui;
struct tsgl_gui {
    void* target;
    bool buffered;

    size_t parentsCount;
    tsgl_gui** parents;
    tsgl_gui* parent;

    tsgl_pos x;
    tsgl_pos y;
    tsgl_pos width;
    tsgl_pos height;

    bool interactive;
    bool displayable;

    void* data;
};

tsgl_gui* tsgl_gui_createRoot_display(tsgl_display* display);
tsgl_gui* tsgl_gui_createRoot_buffer(tsgl_framebuffer* framebuffer);
tsgl_gui* tsgl_gui_addObject(tsgl_gui* object);
void tsgl_gui_draw(tsgl_gui* object);
void tsgl_gui_free(tsgl_gui* object);