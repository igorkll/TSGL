#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_font.h"

typedef struct {
    tsgl_rawcolor bg;
    tsgl_print_settings sets;
    const char* text;
    bool freeText;
} tsgl_gui_textData;

tsgl_gui* tsgl_gui_addText(tsgl_gui* gui);