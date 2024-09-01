#include "colorpicker.h"

static void _math_callback(tsgl_gui* self) {
    tsgl_gui_colorpickerData* data = self->data;

    data->baseWidth = TSGL_MATH_MIN(self->math_width, self->math_height);
    if (data->baseWidth >= self->math_width) {
        data->baseWidth /= 1.2;
    }
    if (data->pointerPosX < 0) {
        data->pointerPosX = data->baseWidth - 1;
    }
}

static void _event_callback(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {
    tsgl_gui_colorpickerData* data = self->data;

    switch (event) {
        // fall through
        case tsgl_gui_click:
            if (x >= data->baseWidth) {
                data->selectedZone = 1;
            } else {
                data->selectedZone = 2;
            }

        case tsgl_gui_drag:
            if (data->selectedZone > 0) {
                data->oldPointerPosX = data->pointerPosX;
                data->oldPointerPosY = data->pointerPosY;
                data->oldHuePointerPos = data->huePointerPos;
                if (data->selectedZone == 1) {
                    if (y != data->huePointerPos) {
                        data->huePointerPos = TSGL_MATH_CLAMP(y, 0, self->math_height - 1);
                        data->hue = tsgl_math_imap(data->huePointerPos, 0, self->math_height - 1, 0, 255);
                        data->svUpdateFlag = true;
                        self->needDraw = true;
                    }
                } else {
                    if (x != data->pointerPosX && y != data->pointerPosY) {
                        data->pointerPosX = TSGL_MATH_CLAMP(x, 0, data->baseWidth - 1);
                        data->pointerPosY = TSGL_MATH_CLAMP(y, 0, self->math_height - 1);
                        data->saturation = tsgl_math_imap(data->pointerPosX, 0, data->baseWidth - 1, 0, 255);
                        data->value = tsgl_math_imap(data->pointerPosY, 0, self->math_height - 1, 255, 0);
                        self->needDraw = true;
                    }
                    
                }
            }
            break;

        case tsgl_gui_drop:
            data->selectedZone = 0;
            break;
        
        default:
            break;
    }
}

static void _drawStuff(tsgl_gui* self, bool hue, bool sv) {
    tsgl_gui_colorpickerData* data = self->data;

    for (tsgl_pos iy = 0; iy < self->math_height; iy++) {
        if (sv) {
            for (tsgl_pos ix = 0; ix < data->baseWidth; ix++) {
                TSGL_GUI_DRAW(self, set, self->math_x + ix, self->math_y + iy, tsgl_color_raw(
                    tsgl_color_hsv(data->hue, tsgl_math_imap(ix, 0, data->baseWidth - 1, 0, 255), tsgl_math_imap(iy, 0, self->math_height - 1, 255, 0)),
                self->colormode));
            }
        }

        if (hue) {
            TSGL_GUI_DRAW(self, fill, self->math_x + data->baseWidth, self->math_y + iy, self->math_width - data->baseWidth, 1, tsgl_color_raw(
                tsgl_color_hsv(tsgl_math_imap(iy, 0, self->math_height - 1, 0, 255), 255, 255),
            self->colormode));
        }
    }
}

static void _fast_draw_callback(tsgl_gui* self) {
    tsgl_gui_colorpickerData* data = self->data;

    _drawStuff(self, false, data->svUpdateFlag);
    data->svUpdateFlag = false;

    tsgl_pos oldpos = TSGL_MATH_CLAMP(data->oldHuePointerPos, 0, self->math_height - 5);
    tsgl_pos pos = TSGL_MATH_CLAMP(data->huePointerPos, 0, self->math_height - 5);
    for (tsgl_pos iy = oldpos; iy < oldpos + 5; iy++) {
        TSGL_GUI_DRAW(self, fill, self->math_x + data->baseWidth, self->math_y + iy, self->math_width - data->baseWidth, 1, tsgl_color_raw(
            tsgl_color_hsv(tsgl_math_imap(iy, 0, self->math_height - 1, 0, 255), 255, 255),
        self->colormode));
    }

    TSGL_GUI_DRAW(self, set, self->math_x + data->oldPointerPosX, self->math_y + data->oldPointerPosY, tsgl_color_raw(
        tsgl_color_hsv(data->hue, tsgl_math_imap(data->oldPointerPosX, 0, data->baseWidth - 1, 0, 255), tsgl_math_imap(data->oldPointerPosY, 0, self->math_height - 1, 255, 0)),
    self->colormode));

    TSGL_GUI_DRAW(self, fill, self->math_x + data->baseWidth, self->math_y + pos, self->math_width - data->baseWidth, 5, tsgl_color_raw(TSGL_WHITE, self->colormode));
    TSGL_GUI_DRAW(self, set, self->math_x + data->pointerPosX, self->math_y + data->pointerPosY, tsgl_color_raw(TSGL_WHITE, self->colormode));
}

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_colorpickerData* data = self->data;

    _drawStuff(self, true, true);
    data->svUpdateFlag = false;

    _fast_draw_callback(self);
}

tsgl_gui* tsgl_gui_addColorpicker(tsgl_gui* gui) {
    tsgl_gui_colorpickerData* data = calloc(1, sizeof(tsgl_gui_colorpickerData));
    data->hue = 0;
    data->saturation = 255;
    data->value = 255;
    data->pointerPosX = -1;

    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->data = data;
    obj->math_callback = _math_callback;
    obj->event_callback = _event_callback;
    obj->draw_callback = _draw_callback;
    obj->fast_draw_callback = _fast_draw_callback;
    return obj;
}

tsgl_color tsgl_gui_colorpicker_getColor(tsgl_gui* colorpicker) {
    return TSGL_BLACK;
}