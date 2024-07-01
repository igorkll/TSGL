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
    bool draggable; //allows elements to move in the space of the parent element. to work, the object must use an absolute position

    tsgl_rawcolor color; //if you set this color, the call to the object rendering callback will stop, however, if you set the "redefining_color" flag, this field will be ignored by the gui and can be used to configure the object
    bool redefining_color;

    // callbacks
    void (*create_callback)(tsgl_gui* self);
    void (*event_callback)(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event);
    void (*draw_callback)(tsgl_gui* self);
    void (*free_callback)(tsgl_gui* self);

    // touchscreen state
    bool pressed;
    tsgl_pos tx;
    tsgl_pos ty;
    tsgl_pos tpx;
    tsgl_pos tpy;
    tsgl_pos tdx;
    tsgl_pos tdy;

    // internal
    tsgl_display* display;
    void* target;
    bool buffered;
    tsgl_colormode colormode;

    bool needMath;
    bool needDraw;

    bool localMovent;
    tsgl_pos old_math_x;
    tsgl_pos old_math_y;
    tsgl_pos math_x;
    tsgl_pos math_y;
    tsgl_pos math_width;
    tsgl_pos math_height;
    tsgl_pos offsetX;
    tsgl_pos offsetY;

    size_t childrenCount;
    tsgl_gui** children;
    tsgl_gui* parent;
    tsgl_gui* root;

    void* data;
    int intData;
};

tsgl_gui* tsgl_gui_createRoot_display(tsgl_display* display, tsgl_colormode colormode); //you specify the colormode yourself because on some screens it must be different from the colormode specified in the driver
tsgl_gui* tsgl_gui_createRoot_buffer(tsgl_display* display, tsgl_framebuffer* framebuffer);
tsgl_gui* tsgl_gui_addObject(tsgl_gui* object);
void tsgl_gui_free(tsgl_gui* object);

// call it in a perpetual loop for the gui to work
void tsgl_gui_processTouchscreen(tsgl_gui* root, tsgl_touchscreen* touchscreen);
void tsgl_gui_processGui(tsgl_gui* root, tsgl_framebuffer* asyncFramebuffer);