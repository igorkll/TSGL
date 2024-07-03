#include "text.h"

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_textData* data = self->data;
    if (!data->bg.invalid) {
        TSGL_GUI_DRAW(self, fill, self->math_x, self->math_y, self->math_width, self->math_height, data->bg);
    }
    TSGL_GUI_DRAW(self, text, self->math_x, self->math_y, data->sets, data->text);
}

static void _free_callback(tsgl_gui* self) {
    tsgl_gui_textData* data = self->data;
    if (data->freeText) free(data->text);
}

tsgl_gui* tsgl_gui_addText(tsgl_gui* gui) {
    tsgl_gui* obj = tsgl_gui_addObject(gui);
    tsgl_gui_textData* data = calloc(1, sizeof(tsgl_gui_textData));
    data->bg = TSGL_INVALID_RAWCOLOR;
    obj->data = data;
    obj->draw_callback = _draw_callback;
    obj->free_callback = _free_callback;
    return obj;
}