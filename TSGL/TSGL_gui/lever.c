#include "lever.h"

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_leverData* data = self->data;
    TSGL_GUI_DRAW(self, fill, self->math_x, self->math_y, self->math_width, self->math_height, self->intData == 1 ? data->enable_body : data->disable_body);
    TSGL_GUI_DRAW(self, fill, self->math_x + (self->intData == 1 ? (self->math_width - self->math_height) : 0), self->math_y, self->math_height, self->math_height, self->intData == 1 ? data->enable_level : data->disable_level);
}

static void _event_callback(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {
    switch (event) {
        case tsgl_gui_click:
            self->intData = !self->intData;
            printf("CGB %i\n", self->intData);
            self->needDraw = true;
            if (self->user_callback != NULL) self->user_callback(self, self->intData, NULL, self->userArg);
            break;

        default:
            break;
    }
}

tsgl_gui* tsgl_gui_addLever(tsgl_gui* gui, bool state) {
    tsgl_gui_leverData* data = calloc(1, sizeof(tsgl_gui_leverData));
    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->draw_callback = _draw_callback;
    obj->event_callback = _event_callback;
    obj->data = data;
    tsgl_gui_lever_setState(obj, state);
    tsgl_gui_lever_setParams(obj, tsgl_color_raw(tsgl_color_fromHex(0x606060), gui->colormode), tsgl_color_raw(tsgl_color_fromHex(0xaeaeae), gui->colormode), tsgl_color_raw(tsgl_color_fromHex(0xaeaeae), gui->colormode), tsgl_color_raw(tsgl_color_fromHex(0x00ff00), gui->colormode));
    return obj;
}

bool tsgl_gui_lever_getState(tsgl_gui* self) {
    return self->intData;
}

void tsgl_gui_lever_setState(tsgl_gui* self, bool state) {
    self->intData = state;
}

void tsgl_gui_lever_setParams(tsgl_gui* self, tsgl_rawcolor disable_body, tsgl_rawcolor disable_level, tsgl_rawcolor enable_body, tsgl_rawcolor enable_level) {
    tsgl_gui_leverData* data = self->data;
    data->disable_body = disable_body;
    data->disable_level = disable_level;
    data->enable_body = enable_body;
    data->enable_level = enable_level;
}