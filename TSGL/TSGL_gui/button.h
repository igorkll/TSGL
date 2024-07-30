#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_color.h"
#include "../TSGL_math.h"
#include "text.h"

typedef struct {
    tsgl_color color;
    tsgl_color pressedColor;
} tsgl_gui_buttonData;

tsgl_gui* tsgl_gui_addButton(tsgl_gui* gui, tsgl_color color);
void tsgl_gui_button_setEmpty(tsgl_gui* button);
void tsgl_gui_button_setText(tsgl_gui* button, tsgl_color textColor, tsgl_pos targetWidth, const char* text, bool freeText);