#include "text.h"

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_textData* data = self->data;
    if (data->text != NULL) {
        data->sets.width = self->math_width;
        data->sets.height = self->math_height;
        TSGL_GUI_DRAW(self, text, self->math_x, self->math_y, data->sets, data->text);
    }
}

static void _free_callback(tsgl_gui* self) {
    tsgl_gui_textData* data = self->data;
    if (data->freeText) free((void*)data->text);
}

tsgl_gui* tsgl_gui_addText(tsgl_gui* gui) {
    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->interactive = false;
    obj->draw_callback = _draw_callback;
    obj->free_callback = _free_callback;
    
    tsgl_gui_textData* data = calloc(1, sizeof(tsgl_gui_textData));
    data->sets.fill = TSGL_INVALID_RAWCOLOR;
    data->sets.bg = TSGL_INVALID_RAWCOLOR;
    data->sets.fg = tsgl_color_raw(TSGL_WHITE, obj->colormode);
    data->sets.font = tsgl_font_defaultFont;
    data->sets.locationMode = tsgl_print_start_top;
    data->sets.multiline = true;
    data->sets.globalCentering = true;

    obj->data = data;
    return obj;
}

void tsgl_gui_text_setText(tsgl_gui* self, const char* text, bool freeText) {
    tsgl_gui_textData* data = self->data;
    if (data->freeText) free((void*)data->text);
    data->freeText = freeText;
    data->text = text;
}

void tsgl_gui_text_setParams(tsgl_gui* self, tsgl_print_settings sets) {
    tsgl_gui_textData* data = self->data;
    data->sets = sets;
}

tsgl_print_textArea tsgl_gui_text_getTextArea(tsgl_gui* self) {
    tsgl_gui_textData* data = self->data;
    return tsgl_font_getTextArea(tsgl_gui_mathObjectPos(self, false), tsgl_gui_mathObjectPos(self, true), data->sets, data->text);
}