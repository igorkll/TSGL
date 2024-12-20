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
    tsgl_gui* tabObject;
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

typedef struct {
    tsgl_rawcolor color;
    tsgl_rawcolor selectedColor;
    tsgl_gui* button;
    tsgl_gui_tabbarData* tabbarData;
} tsgl_gui_ewq;

tsgl_gui* tsgl_gui_addTabbar(tsgl_gui* gui, bool horizontal, tsgl_pos padding, tsgl_pos offset, tsgl_pos buttonSize);
tsgl_gui* tsgl_gui_tabbar_addTabObject(tsgl_gui* self, bool negSide); //creates a tab object, stretches it to the entire available space relative to the tabbar
tsgl_gui* tsgl_gui_tabbar_addTabButton(tsgl_gui* self, tsgl_color color, tsgl_color selectedColor, tsgl_gui* tabObject); //returns the button for the tab
void tsgl_gui_tabbar_select(tsgl_gui* self);