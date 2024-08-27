#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_font.h"

typedef struct {
    tsgl_print_settings sets;
    const char* text;
    bool freeText;
} tsgl_gui_textData;

tsgl_gui* tsgl_gui_addText(tsgl_gui* gui);
void tsgl_gui_text_setText(tsgl_gui* self, const char* text, bool freeText);
void tsgl_gui_text_setParams(tsgl_gui* self, tsgl_print_settings sets);