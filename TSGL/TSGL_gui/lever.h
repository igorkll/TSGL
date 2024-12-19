#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_font.h"

typedef struct {
    tsgl_rawcolor disable_body;
    tsgl_rawcolor disable_level;
    tsgl_rawcolor enable_body;
    tsgl_rawcolor enable_level;
} tsgl_gui_leverData;

tsgl_gui* tsgl_gui_addLever(tsgl_gui* gui, bool state);
bool tsgl_gui_lever_getState(tsgl_gui* self);
void tsgl_gui_lever_setState(tsgl_gui* self, bool state);
void tsgl_gui_lever_setParams(tsgl_gui* self, tsgl_rawcolor disable_body, tsgl_rawcolor disable_level, tsgl_rawcolor enable_body, tsgl_rawcolor enable_level);