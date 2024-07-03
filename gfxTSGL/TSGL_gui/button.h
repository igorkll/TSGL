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
void tsgl_gui_button_setText(tsgl_gui* self, const char* text, bool freeText);
void tsgl_gui_button_setTextParams(tsgl_gui* self, const void* font, float scale, tsgl_color background, tsgl_color foreground);