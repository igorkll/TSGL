#include "colorpicker.h"

static void _event_callback(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {

}

static void _draw_callback(tsgl_gui* self) {

}

static void _fast_draw_callback(tsgl_gui* self) {

}

tsgl_gui* tsgl_gui_addColorpicker(tsgl_gui* gui) {
    tsgl_gui_colorpickerData* data = calloc(1, sizeof(tsgl_gui_colorpickerData));
    data->hue = 1;
    data->saturation = 1;
    data->value = 0;

    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->data = data;
    obj->event_callback = _event_callback;
    obj->draw_callback = _draw_callback;
    obj->fast_draw_callback = _fast_draw_callback;
    return obj;
}

void tsgl_gui_colorpicker_getColor(tsgl_gui* colorpicker) {

}