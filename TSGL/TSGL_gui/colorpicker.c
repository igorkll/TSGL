#include "colorpicker.h"

static void _math_callback(tsgl_gui* self) {
    tsgl_gui_colorpickerData* data = self->data;

    data->baseWidth = TSGL_MATH_MIN(self->math_width, self->math_height);
    if (data->baseWidth >= self->math_width) {
        data->baseWidth /= 1.2;
    }
}

static void _event_callback(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {

}

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_colorpickerData* data = self->data;

    TSGL_GUI_DRAW(self, fill, self->math_x, self->math_y, self->math_width, self->math_height,
        tsgl_color_raw(TSGL_WHITE, self->colormode)
    );

    for (tsgl_pos iy = 0; iy < self->math_height; iy++) {
        for (tsgl_pos ix = 0; ix < data->baseWidth; ix++) {
            TSGL_GUI_DRAW(self, set, self->math_x + ix, self->math_y + iy, tsgl_color_raw(
                tsgl_color_hsv(data->hue, tsgl_math_imap(ix, 0, data->baseWidth - 1, 0, 255), tsgl_math_imap(iy, 0, self->math_height - 1, 255, 0)),
            self->colormode));
        }

        tsgl_pos huePos = data->baseWidth + 1;
        TSGL_GUI_DRAW(self, fill, self->math_x + huePos, self->math_y + iy, self->math_width - huePos, 1, tsgl_color_raw(
            tsgl_color_hsv(tsgl_math_imap(iy, 0, self->math_height - 1, 0, 255), data->saturation, data->value),
        self->colormode));
    }
}

tsgl_gui* tsgl_gui_addColorpicker(tsgl_gui* gui) {
    tsgl_gui_colorpickerData* data = calloc(1, sizeof(tsgl_gui_colorpickerData));
    data->hue = 0;
    data->saturation = 255;
    data->value = 255;

    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->data = data;
    obj->math_callback = _math_callback;
    obj->event_callback = _event_callback;
    obj->draw_callback = _draw_callback;
    return obj;
}

void tsgl_gui_colorpicker_getColor(tsgl_gui* colorpicker) {

}