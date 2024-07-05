#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_color.h"
#include "text.h"

typedef struct {
    tsgl_gui* text;
    tsgl_color color;
    tsgl_color pressedColor;
} tsgl_gui_buttonData;

tsgl_gui* tsgl_gui_addButton(tsgl_gui* gui);
tsgl_gui* tsgl_gui_button_getTextChild(tsgl_gui* self);