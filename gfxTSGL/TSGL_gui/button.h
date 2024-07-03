#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "text.h"

typedef struct {
    tsgl_gui_textData* textData;
    tsgl_rawcolor bg;
} tsgl_gui_buttonData;

tsgl_gui* tsgl_gui_addButton(tsgl_gui* gui);