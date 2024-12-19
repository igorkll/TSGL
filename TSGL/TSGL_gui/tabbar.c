#include "tabbar.h"

static void _free_callback(tsgl_gui* self) {
    tsgl_gui_tabbarData* data = self->data;
    for (size_t i = 0; i < data->tabsCount; i++) {
        free(data->tabs[i]);
    }
    free(data->tabs);
}

static void* _onTabChange(tsgl_gui* self, int arg0, void* arg1, void* userArg) {

}

tsgl_gui* tsgl_gui_addTabbar(tsgl_gui* gui, bool horizontal, tsgl_pos padding, tsgl_pos offset, tsgl_pos buttonSize) {
    tsgl_gui_tabbarData* data = malloc(sizeof(tsgl_gui_tabbarData));
    data->horizontal = horizontal;
    data->padding = padding;
    data->offset = offset;
    data->buttonSize = buttonSize;
    data->tabs = NULL;
    data->tabsCount = 0;
    data->currentPos = 0;

    tsgl_gui* tabbar = tsgl_gui_addObject(gui);
    tabbar->data = data;
    tabbar->free_callback = _free_callback;
    return tabbar;
}

tsgl_gui* tsgl_gui_tabbar_addTab(tsgl_gui* self, tsgl_color color, tsgl_color selectedColor) {
    tsgl_gui_tabbarData* data = self->data;
    data->tabsCount++;
    if (object->tabs == NULL) {
        object->tabs = malloc(object->tabsCount * sizeof(size_t));
    } else {
        object->tabs = realloc(object->tabs, object->tabsCount * sizeof(size_t));
    }

    tsgl_gui* tabButton = tsgl_gui_addButton(self);
    tabButton->animationStopEnable = true;
    tabButton->animationStop = 0;
    tabButton->user_callback = _onTabChange;
    if (data->horizontal) {
        tabButton->x = data->padding + data->currentPos;
        tabButton->y = data->padding;
        tabButton->width = data->buttonSize;
        tabButton->height = tsgl_gui_mathObjectPos(self, true) - (data->padding * 2);
    } else {
        tabButton->x = data->padding;
        tabButton->y = data->padding + data->currentPos;
        tabButton->width = tsgl_gui_mathObjectPos(self, false) - (data->padding * 2);
        tabButton->height = data->buttonSize;
    }
    data->currentPos += data->buttonSize + data->offset;

    tsgl_gui_tabbarData_tab* tabData = malloc(sizeof(tsgl_gui_tabbarData_tab));
    tabData->color = color;
    tabData->selectedColor = selectedColor;
    tabData->button = tabButton;
    tabData->tabbarData = data;
    object->tabs[object->tabsCount - 1] = tabData;
    return tabButton;
}

tsgl_gui* tsgl_gui_tabbar_select(tsgl_gui* self) {
    
}