#pragma once
#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"

typedef enum {
    tsgl_gui_absolute,
    tsgl_gui_percent
} tsgl_gui_paramFormat;

typedef enum {
    tsgl_gui_click,
    tsgl_gui_drag,
    tsgl_gui_drop
} tsgl_gui_event;

typedef struct tsgl_gui tsgl_gui;
struct tsgl_gui {
    // settings
    tsgl_gui_paramFormat format_x; float x;
    tsgl_gui_paramFormat format_y; float y;
    tsgl_gui_paramFormat format_width; float width;
    tsgl_gui_paramFormat format_height; float height;

    bool interactive;
    bool displayable;
    bool draggable; //allows elements to move in the space of the parent element

    // callbacks
    void (*create_callback)(tsgl_gui* self);
    void (*event_callback)(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event);
    void (*draw_callback)(tsgl_gui* self);
    void (*free_callback)(tsgl_gui* self);

    // touchscreen state
    bool pressed;
    tsgl_pos tx;
    tsgl_pos ty;

    // internal
    void* target;
    bool buffered;
    tsgl_colormode colormode;

    bool needMath;
    bool needDraw;

    tsgl_pos math_x;
    tsgl_pos math_y;
    tsgl_pos math_width;
    tsgl_pos math_height;

    size_t parentsCount;
    tsgl_gui** parents;
    tsgl_gui* parent;
    tsgl_gui* root;

    void* data;
    int intData;
    bool data_as_callback;
};

tsgl_gui* tsgl_gui_createRoot_display(tsgl_display* display, tsgl_colormode colormode); //you specify the colormode yourself because on some screens it must be different from the colormode specified in the driver
tsgl_gui* tsgl_gui_createRoot_buffer(tsgl_framebuffer* framebuffer);
tsgl_gui* tsgl_gui_addObject(tsgl_gui* object);
void tsgl_gui_free(tsgl_gui* object);

// a method for controlling the background, be sure to call one of them after creating the root gui object
void tsgl_gui_setClearColor(tsgl_gui* root, tsgl_rawcolor color);
void tsgl_gui_attachClearCallback(tsgl_gui* root, bool free_arg, void* arg, void (*onClear)(tsgl_gui* root, void* arg)); //set free_arg to true to automatically make free on the argument after calling free for the object or calling tsgl_gui_attachClearCallback again

// these methods are mostly for internal use
void tsgl_gui_math(tsgl_gui* root);
void tsgl_gui_draw(tsgl_gui* object);

// call it in a perpetual loop for the gui to work
void tsgl_gui_processTouchscreen(tsgl_gui* root, tsgl_touchscreen* touchscreen);
void tsgl_gui_processGui(tsgl_gui* root, void* arg, void (*onDraw)(tsgl_gui* root, void* arg)); //the callback will be called with your argument in cases where the redrawing has occurred