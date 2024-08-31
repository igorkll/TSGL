#include "colorpicker.h"

static void _math_callback(tsgl_gui* self) {
    tsgl_gui_colorpickerData* data = self->data;

    data->baseWidth = TSGL_MATH_MIN(self->math_width, self->math_height);
    if (data->baseWidth >= self->math_width) {
        data->baseWidth /= 1.2;
    }
}

static void _event_callback(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {
    tsgl_gui_colorpickerData* data = self->data;

    switch (event) {
        case tsgl_gui_click:
        case tsgl_gui_drag:
            data->oldPointerPosX = data->pointerPosX;
            data->oldPointerPosY = data->pointerPosY;
            data->oldHuePointerPos = data->huePointerPos;
            if (x >= data->baseWidth) {
                if (y != data->huePointerPos) {
                    data->huePointerPos = y;
                    self->needDraw = true;
                }
            } else {
                if (x != data->pointerPosX && y != data->pointerPosY) {
                    data->pointerPosX = x;
                    data->pointerPosY = y;
                    self->needDraw = true;
                }
            }
            data->pointerPosX = TSGL_MATH_CLAMP(data->pointerPosX, 0, data->baseWidth - 1);
            data->pointerPosY = TSGL_MATH_CLAMP(data->pointerPosY, 0, self->math_height - 1);
            data->huePointerPos = TSGL_MATH_CLAMP(data->huePointerPos, 0, self->math_height - 1);
            break;
        
        default:
            break;
    }
}

static void _fast_draw_callback(tsgl_gui* self) {
    tsgl_gui_colorpickerData* data = self->data;
    tsgl_pos oldpos = TSGL_MATH_CLAMP(data->oldHuePointerPos, 2, self->math_height - 2);
    tsgl_pos pos = TSGL_MATH_CLAMP(data->huePointerPos, 2, self->math_height - 2);

    TSGL_GUI_DRAW(self, set, self->math_x + data->oldPointerPosX, self->math_y + data->oldPointerPosY, tsgl_color_raw(
        tsgl_color_hsv(data->hue, tsgl_math_imap(data->oldPointerPosX, 0, data->baseWidth - 1, 0, 255), tsgl_math_imap(data->oldPointerPosY, 0, self->math_height - 1, 255, 0)),
    self->colormode));
    TSGL_GUI_DRAW(self, fill, self->math_x + data->baseWidth, (self->math_y + oldpos) - 2, self->math_width - data->baseWidth, 5 - abs(data->oldHuePointerPos - oldpos), tsgl_color_raw(TSGL_WHITE, self->colormode));

    TSGL_GUI_DRAW(self, fill, self->math_x + data->baseWidth, (self->math_y + pos) - 2, self->math_width - data->baseWidth, 5 - abs(data->huePointerPos - pos), tsgl_color_raw(TSGL_WHITE, self->colormode));
    TSGL_GUI_DRAW(self, set, self->math_x + data->pointerPosX, self->math_y + data->pointerPosY, tsgl_color_raw(TSGL_WHITE, self->colormode));
}

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_colorpickerData* data = self->data;

    for (tsgl_pos iy = 0; iy < self->math_height; iy++) {
        for (tsgl_pos ix = 0; ix < data->baseWidth; ix++) {
            TSGL_GUI_DRAW(self, set, self->math_x + ix, self->math_y + iy, tsgl_color_raw(
                tsgl_color_hsv(data->hue, tsgl_math_imap(ix, 0, data->baseWidth - 1, 0, 255), tsgl_math_imap(iy, 0, self->math_height - 1, 255, 0)),
            self->colormode));
        }

        TSGL_GUI_DRAW(self, fill, self->math_x + data->baseWidth, self->math_y + iy, self->math_width - data->baseWidth, 1, tsgl_color_raw(
            tsgl_color_hsv(tsgl_math_imap(iy, 0, self->math_height - 1, 0, 255), data->saturation, data->value),
        self->colormode));
    }

    _fast_draw_callback(self);
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
    obj->fast_draw_callback = _fast_draw_callback;
    return obj;
}

void tsgl_gui_colorpicker_getColor(tsgl_gui* colorpicker) {

}