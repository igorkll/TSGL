#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_color.h"
#include "../TSGL_math.h"

typedef struct {
    float hue;
    float saturation;
    float value;
} tsgl_gui_colorpickerData;

tsgl_gui* tsgl_gui_addColorpicker(tsgl_gui* gui);
void tsgl_gui_colorpicker_getColor(tsgl_gui* colorpicker);