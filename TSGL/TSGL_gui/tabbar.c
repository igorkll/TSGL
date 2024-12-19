#include "tabbar.h"
#include "button.h"

static void _free_callback(tsgl_gui* self) {
    tsgl_gui_tabbarData* data = self->data;
    for (size_t i = 0; i < data->tabsCount; i++) {
        free(data->tabs[i]);
    }
    free(data->tabs);
}

static void* _onTabChange(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0 == 1) {
        tsgl_gui_tabbar_select(self);
    }
    return NULL;
}

tsgl_gui* tsgl_gui_addTabbar(tsgl_gui* gui, bool horizontal, tsgl_pos padding, tsgl_pos offset, tsgl_pos buttonSize) {
    tsgl_gui_tabbarData* data = malloc(sizeof(tsgl_gui_tabbarData));
    data->horizontal = horizontal;
    data->padding = padding;
    data->offset = offset;
    data->buttonSize = buttonSize;
    data->tabs = NULL;
    data->oldTab = NULL;
    data->tabsCount = 0;
    data->currentPos = 0;

    tsgl_gui* tabbar = tsgl_gui_addObject(gui);
    tabbar->data = data;
    tabbar->free_callback = _free_callback;
    return tabbar;
}

tsgl_gui* tsgl_gui_tabbar_addTab(tsgl_gui* self, tsgl_color color, tsgl_color selectedColor, tsgl_gui* tabObject) {
    tsgl_gui_tabbarData* data = self->data;
    data->tabsCount++;
    if (data->tabs == NULL) {
        data->tabs = malloc(data->tabsCount * sizeof(size_t));
    } else {
        data->tabs = realloc(data->tabs, data->tabsCount * sizeof(size_t));
    }

    tabObject->interactive = false;
    tabObject->displayable = false;

    tsgl_gui* tabButton = tsgl_gui_addButton(self);
    tabButton->color = tsgl_color_raw(color, self->colormode);
    tabButton->animationStopEnable = true;
    tabButton->animationStop = 0;
    tabButton->user_callback = _onTabChange;
    if (data->horizontal) {
        tabButton->x = data->padding + data->currentPos;
        tabButton->y = data->padding;
        tabButton->width = data->buttonSize;
        tabButton->height = tsgl_gui_mathObjectSize(self, true) - (data->padding * 2);
    } else {
        tabButton->x = data->padding;
        tabButton->y = data->padding + data->currentPos;
        tabButton->width = tsgl_gui_mathObjectSize(self, false) - (data->padding * 2);
        tabButton->height = data->buttonSize;
    }
    data->currentPos += data->buttonSize + data->offset;

    tsgl_gui_tabbarData_tab* tabData = malloc(sizeof(tsgl_gui_tabbarData_tab));
    tabData->color = tsgl_color_raw(color, self->colormode);
    tabData->selectedColor = tsgl_color_raw(selectedColor, self->colormode);
    tabData->button = tabButton;
    tabData->tabbarData = data;
    tabData->tabObject = tabObject;
    data->tabs[data->tabsCount - 1] = tabData;
    tabButton->userArg = tabData;

    if (data->tabsCount == 1) {
        tsgl_gui_tabbar_select(tabButton);
    }

    return tabButton;
}

void tsgl_gui_tabbar_select(tsgl_gui* self) {
    tsgl_gui_tabbarData_tab* tabData = self->userArg;
    tsgl_gui_tabbarData* tabbarData = tabData->tabbarData;
    if (tabbarData->oldTab) {
        tabbarData->oldTab->button->color = tabData->color;
        tabbarData->oldTab->button->needDraw = true;
        tabbarData->oldTab->tabObject->interactive = false;
        tabbarData->oldTab->tabObject->displayable = false;
    }
    tabData->button->color = tabData->selectedColor;
    tabData->button->needDraw = true;
    tabData->tabObject->interactive = true;
    tabData->tabObject->displayable = true;
    tabData->tabObject->needDraw = true;
    tabbarData->oldTab = tabData;
}