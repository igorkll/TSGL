#pragma once
#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_benchmark.h"
#include "TSGL_display.h"

#define TSGL_GUI_DRAW(gui, name, ...) if (gui->buffered) tsgl_framebuffer_##name(gui->target, __VA_ARGS__); else tsgl_display_##name(gui->target, __VA_ARGS__)

typedef enum {
    tsgl_gui_absolute,
    tsgl_gui_percent,
    tsgl_gui_percentMinSide, //uses a smaller scale even for the larger side
    tsgl_gui_percentMaxSide //uses the maximum side, including for the smaller scale
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
    bool leaky_walls; //if the value is true, the child elements with the "draggable" flag will be able to go beyond the boundaries of this element. by default, the root element has true. may cause problems if this flag is set for a non-fullscreen element

    tsgl_rawcolor color; //if you set this color, instead of rendering the object, it will be filled with a rectangle of a certain color

    // callbacks
    void* (*user_callback)(tsgl_gui* self, int arg0, void* arg1, void* userArg);
    void (*event_callback)(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event);
    void (*draw_callback)(tsgl_gui* self);
    void (*free_callback)(tsgl_gui* self);
    void* userArg;

    // touchscreen state
    bool pressed;
    tsgl_pos tx;
    tsgl_pos ty;
    tsgl_pos tpx;
    tsgl_pos tpy;
    tsgl_pos tdx;
    tsgl_pos tdy;

    // animation
    float animationState;
    float animationTarget;
    float animationSpeed; //the number of seconds that the animation should run
    float animationSpeedUpMul;
    float animationSpeedDownMul;

    // internal
    tsgl_display* display;
    void* target;
    bool buffered;
    tsgl_colormode colormode;

    bool fillParentSize;
    bool needMath;
    bool needDraw;
    bool drawLater;
    bool drawLaterLater;

    bool processing;
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

    bool noFreeData;
    void* data;
    int intData;
};

tsgl_gui* tsgl_gui_createRoot_display(tsgl_display* display, tsgl_colormode colormode); //you specify the colormode yourself because on some screens it must be different from the colormode specified in the driver
tsgl_gui* tsgl_gui_createRoot_buffer(tsgl_display* display, tsgl_framebuffer* framebuffer);
tsgl_gui* tsgl_gui_addObject(tsgl_gui* object);
void tsgl_gui_free(tsgl_gui* object);

// these are just shortcuts in order to set formats in one line
void tsgl_gui_setPosFormat(tsgl_gui* object, tsgl_gui_paramFormat format);
void tsgl_gui_setScaleFormat(tsgl_gui* object, tsgl_gui_paramFormat format);
void tsgl_gui_setAllFormat(tsgl_gui* object, tsgl_gui_paramFormat format);

// call it in a perpetual loop for the gui to work
void tsgl_gui_processClick(tsgl_gui* obj, tsgl_pos x, tsgl_pos y, tsgl_gui_event clickType);
void tsgl_gui_processTouchscreen(tsgl_gui* root, tsgl_touchscreen* touchscreen);
void tsgl_gui_processGui(tsgl_gui* root, tsgl_framebuffer* asyncFramebuffer, tsgl_benchmark* benchmark);