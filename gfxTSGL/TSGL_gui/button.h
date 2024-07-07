#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_color.h"
#include "../TSGL_math.h"

typedef struct {
    tsgl_color color;
    tsgl_color pressedColor;
} tsgl_gui_buttonData;

tsgl_gui* tsgl_gui_addButton(tsgl_gui* gui, tsgl_color color);