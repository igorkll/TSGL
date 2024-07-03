#include "text.h"

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_textData* data = self->data;
    if (!data->bg.invalid) {
        TSGL_GUI_DRAW(self, fill, self->math_x, self->math_y, self->math_width, self->math_height, data->bg);
    }
    if (data->text != NULL) {
        TSGL_GUI_DRAW(self, text, self->math_x, self->math_y, data->sets, data->text);
    }
}

static void _free_callback(tsgl_gui* self) {
    tsgl_gui_textData* data = self->data;
    if (data->freeText) free(data->text);
}

tsgl_gui* tsgl_gui_addText(tsgl_gui* gui) {
    tsgl_gui* obj = tsgl_gui_addObject(gui);
    tsgl_gui_textData* data = calloc(1, sizeof(tsgl_gui_textData));
    data->bg = TSGL_INVALID_RAWCOLOR;
    data->sets.bg = TSGL_INVALID_RAWCOLOR;
    data->sets.fg = tsgl_color_raw(TSGL_WHITE, obj->colormode);
    data->sets.font = tsgl_font_defaultFont;
    data->sets.locationMode = tsgl_print_start_top;
    obj->interactive = false;
    obj->data = data;
    obj->draw_callback = _draw_callback;
    obj->free_callback = _free_callback;
    return obj;
}

void tsgl_gui_text_setText(tsgl_gui* self, const char* text, bool freeText) {
    tsgl_gui_textData* data = self->data;
    if (data->freeText) free(data->text);
    data->freeText = freeText;
    data->text = text;
}

void tsgl_gui_text_setTextParams(tsgl_gui* self, const void* font, float scale, tsgl_color background, tsgl_color foreground) {
    tsgl_gui_textData* data = self->data;
    data->sets.font = font;
    data->bg = tsgl_color_raw(background, self->colormode);
    data->sets.fg = tsgl_color_raw(foreground, self->colormode);
}