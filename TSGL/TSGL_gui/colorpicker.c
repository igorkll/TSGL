#include "colorpicker.h"

static void _math_callback(tsgl_gui* self) {
    tsgl_gui_colorpickerData* data = self->data;

    data->baseWidth = TSGL_MATH_MIN(self->math_width, self->math_height);
    if (data->baseWidth >= self->math_width) {
        data->baseWidth /= 1.2;
    }
    if (data->pointerPosX == -1) {
        data->baseWidth = TSGL_MATH_MIN(self->math_width, self->math_height);
        if (data->baseWidth >= self->math_width) {
            data->baseWidth /= 1.2;
        }
        data->pointerPosX = data->baseWidth - 1;
    }
}

static void _event_callback(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {
    tsgl_gui_colorpickerData* data = self->data;

    switch (event) {
        case tsgl_gui_click:
            if (x >= data->baseWidth) {
                data->selectedZone = 1;
            } else {
                data->selectedZone = 2;
            }
            // fall through

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
            data->color = tsgl_color_hsv(data->hue, data->saturation, data->value);
            if (self->user_callback != NULL) self->user_callback(self, 0, &data->color, self->userArg);
            break;
        
        default:
            break;
    }
}

static void _drawStuff(tsgl_gui* self, bool hue, bool sv) {
    tsgl_gui_colorpickerData* data = self->data;

    if (sv) {
        for (tsgl_pos iy = 0; iy < self->math_height; iy += data->groupPixels) {
            for (tsgl_pos ix = 0; ix < data->baseWidth; ix += data->groupPixels) {
                TSGL_GUI_DRAW(self, fill, self->math_x + ix, self->math_y + iy, data->groupPixels, data->groupPixels, tsgl_color_raw(
                    tsgl_color_hsv(data->hue, tsgl_math_imap(ix, 0, data->baseWidth - 1, 0, 255), tsgl_math_imap(iy, 0, self->math_height - 1, 255, 0)),
                self->colormode));
            }
        }
    }

    if (hue) {
        for (tsgl_pos iy = 0; iy < self->math_height; iy++) {
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

    tsgl_rawcolor selectorColor = tsgl_color_raw(TSGL_WHITE, self->colormode);
    tsgl_rawcolor selectorBgColor = self->display->black;

    uint8_t lsel = data->hueSelectorSize / 2;
    tsgl_viewport_dump dump;
    TSGL_GUI_DRAW(self, dumpViewport, &dump);
    TSGL_GUI_DRAW(self, setViewport, self->math_x + data->baseWidth, self->math_y, self->math_width - data->baseWidth, self->math_height);
    for (tsgl_pos iy = data->oldHuePointerPos - lsel; iy <= data->oldHuePointerPos + lsel; iy++) {
        TSGL_GUI_DRAW(self, fill, self->math_x + data->baseWidth, self->math_y + iy, self->math_width - data->baseWidth, 1, tsgl_color_raw(
            tsgl_color_hsv(tsgl_math_imap(iy, 0, self->math_height - 1, 0, 255), 255, 255),
        self->colormode));
    }
    tsgl_pos x = self->math_x + data->baseWidth;
    tsgl_pos y = (self->math_y + data->huePointerPos) - lsel;
    tsgl_pos sx = self->math_width - data->baseWidth;
    tsgl_pos sy = data->hueSelectorSize;
    TSGL_GUI_DRAW(self, fill, x, y, sx, sy, selectorColor);
    TSGL_GUI_DRAW(self, rect, x, y, sx, sy, selectorBgColor, 1);

    TSGL_GUI_DRAW(self, setViewport, self->math_x, self->math_y, data->baseWidth, self->math_height);
    for (tsgl_pos i = -data->svSelectorSize; i <= data->svSelectorSize; i++) {
        for (tsgl_pos ix = -1; ix <= 1; ix++) {
            for (tsgl_pos iy = -1; iy <= 1; iy++) {
                tsgl_pos x = self->math_x + data->oldPointerPosX + i + ix;
                tsgl_pos y = self->math_y + data->oldPointerPosY + iy;
                TSGL_GUI_DRAW(self, set, x, y + i, tsgl_color_raw(
                    tsgl_color_hsv(data->hue, tsgl_math_imap(data->oldPointerPosX + i + ix, 0, data->baseWidth - 1, 0, 255), tsgl_math_imap(data->oldPointerPosY + i + iy, 0, self->math_height - 1, 255, 0)),
                self->colormode));
                TSGL_GUI_DRAW(self, set, x, y + -i, tsgl_color_raw(
                    tsgl_color_hsv(data->hue, tsgl_math_imap(data->oldPointerPosX + i + ix, 0, data->baseWidth - 1, 0, 255), tsgl_math_imap(data->oldPointerPosY + -i + iy, 0, self->math_height - 1, 255, 0)),
                self->colormode));
            }
        }
    }
    for (tsgl_pos i = -data->svSelectorSize; i <= data->svSelectorSize; i++) {
        tsgl_pos x = self->math_x + data->pointerPosX + i;
        tsgl_pos y = self->math_y + data->pointerPosY;
        TSGL_GUI_DRAW(self, fill, x - 1, (y + i) - 1, 3, 3, selectorBgColor);
        TSGL_GUI_DRAW(self, fill, x - 1, (y + -i) - 1, 3, 3, selectorBgColor);
    }
    for (tsgl_pos i = -data->svSelectorSize; i <= data->svSelectorSize; i++) {
        tsgl_pos x = self->math_x + data->pointerPosX + i;
        tsgl_pos y = self->math_y + data->pointerPosY;
        TSGL_GUI_DRAW(self, set, x, y + i, selectorColor);
        TSGL_GUI_DRAW(self, set, x, y + -i, selectorColor);
    }

    TSGL_GUI_DRAW(self, flushViewport, &dump);
}

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_colorpickerData* data = self->data;

    _drawStuff(self, true, true);
    data->svUpdateFlag = false;

    _fast_draw_callback(self);
}

tsgl_gui* tsgl_gui_addColorpicker(tsgl_gui* gui, tsgl_gui_colorpickerConfig config) {
    tsgl_gui_colorpickerData* data = calloc(1, sizeof(tsgl_gui_colorpickerData));
    data->hue = 0;
    data->saturation = 255;
    data->value = 255;
    data->color = TSGL_RED;
    data->pointerPosX = -1;

    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->data = data;
    obj->math_callback = _math_callback;
    obj->event_callback = _event_callback;
    obj->draw_callback = _draw_callback;
    obj->fast_draw_callback = _fast_draw_callback;

    tsgl_gui_colorpicker_setConfig(obj, config);
    return obj;
}

tsgl_color tsgl_gui_colorpicker_getColor(tsgl_gui* self) {
    tsgl_gui_colorpickerData* data = self->data;
    return data->color;
}

void tsgl_gui_colorpicker_setConfig(tsgl_gui* self, tsgl_gui_colorpickerConfig config) {
    tsgl_gui_colorpickerData* data = self->data;
    data->groupPixels = config.groupPixels;
    data->hueSelectorSize = config.hueSelectorSize;
    data->svSelectorSize = config.svSelectorSize;
    if (data->groupPixels == 0) data->groupPixels = 4;
    if (data->hueSelectorSize == 0) data->hueSelectorSize = 5;
    if (data->svSelectorSize == 0) data->svSelectorSize = 10;
    tsgl_gui_colorpicker_setColor(self, config.color);
}

void tsgl_gui_colorpicker_setColor(tsgl_gui* self, tsgl_color color) {
    tsgl_gui_colorpickerData* data = self->data;
    data->color = color;
}