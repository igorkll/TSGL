#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_color.h"
#include "text.h"

typedef struct {
    tsgl_gui_textData* textData;
    tsgl_color color;
    tsgl_color pressedColor;
} tsgl_gui_buttonData;

tsgl_gui* tsgl_gui_addButton(tsgl_gui* gui);