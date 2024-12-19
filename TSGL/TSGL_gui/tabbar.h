#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_font.h"

typedef struct tsgl_gui_tabbarData tsgl_gui_tabbarData;

typedef struct {
    tsgl_rawcolor color;
    tsgl_rawcolor selectedColor;
    tsgl_gui* button;
    tsgl_gui_tabbarData* tabbarData;
} tsgl_gui_tabbarData_tab;

struct tsgl_gui_tabbarData {
    bool horizontal;
    tsgl_pos currentPos;
    tsgl_pos padding;
    tsgl_pos offset;
    tsgl_pos buttonSize;
    tsgl_gui_tabbarData_tab** tabs;
    tsgl_gui_tabbarData_tab* oldTab;
    size_t tabsCount;
};

tsgl_gui* tsgl_gui_addTabbar(tsgl_gui* gui, bool horizontal, tsgl_pos padding, tsgl_pos offset, tsgl_pos buttonSize);
tsgl_gui* tsgl_gui_tabbar_addTab(tsgl_gui* self, tsgl_color color, tsgl_color selectedColor);
void tsgl_gui_tabbar_select(tsgl_gui* self);