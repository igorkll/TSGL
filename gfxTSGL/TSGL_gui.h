#pragma once
#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"

typedef struct tsgl_gui_object tsgl_gui_object;

struct tsgl_gui_object {
    size_t parentsCount;
    tsgl_gui_object** parents;
    tsgl_gui_object* parent;

    void* target;
    bool buffered;

    tsgl_pos x;
    tsgl_pos y;
    tsgl_pos width;
    tsgl_pos height;

    bool interactive;
    bool displayable;

    void* data;
};

tsgl_gui_object* tsgl_gui_createRoot_display(tsgl_gui* gui, tsgl_display* display);
tsgl_gui_object* tsgl_gui_createRoot_buffer(tsgl_gui* gui, tsgl_framebuffer* framebuffer);
void tsgl_gui_free(tsgl_gui_object* object);