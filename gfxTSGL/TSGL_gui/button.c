#include "button.h"

static void _event_callback(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {
    switch (event) {
        case tsgl_gui_click:
            self->intData = 1;
            self->needDraw = true;
            break;

        case tsgl_gui_drop:
            self->intData = 0;
            self->needDraw = true;
            break;

        default:
            break;
    }
}

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_buttonData* data = self->data;
    TSGL_GUI_DRAW(self, fill, self->math_x, self->math_y, self->math_width, self->math_height,
        self->intData ? tsgl_color_raw(data->pressedColor, self->colormode) : tsgl_color_raw(data->color, self->colormode)
    );
}

tsgl_gui* tsgl_gui_addButton(tsgl_gui* gui) {
    tsgl_gui* obj = tsgl_gui_addObject(gui);
    tsgl_gui* text = tsgl_gui_addText(obj);
    tsgl_gui_buttonData* data = calloc(1, sizeof(tsgl_gui_buttonData));
    data->color = TSGL_BLUE;
    data->pressedColor = TSGL_CYAN;
    data->text = text;
    obj->data = data;
    obj->draw_callback = _draw_callback;
    return obj;
}

void tsgl_gui_button_setText(tsgl_gui* self, const char* text, bool freeText) {
    tsgl_gui_buttonData* data = self->data;
    tsgl_gui_text_setText(data->text, text, freeText);
}

void tsgl_gui_button_setTextParams(tsgl_gui* self, const void* font, float scale, tsgl_color background, tsgl_color foreground) {
    tsgl_gui_buttonData* data = self->data;
    tsgl_gui_text_setTextParams(data->text, font, scale, background, foreground);
}