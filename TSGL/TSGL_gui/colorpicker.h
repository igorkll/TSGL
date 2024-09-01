#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_color.h"
#include "../TSGL_math.h"

typedef struct {
    uint8_t hue;
    uint8_t saturation;
    uint8_t value;
    tsgl_pos baseWidth;
    tsgl_pos pointerPosX;
    tsgl_pos pointerPosY;
    tsgl_pos huePointerPos;
    tsgl_pos oldPointerPosX;
    tsgl_pos oldPointerPosY;
    tsgl_pos oldHuePointerPos;
    uint8_t selectedZone;
    bool svUpdateFlag;
} tsgl_gui_colorpickerData;

tsgl_gui* tsgl_gui_addColorpicker(tsgl_gui* gui);
tsgl_color tsgl_gui_colorpicker_getColor(tsgl_gui* colorpicker);