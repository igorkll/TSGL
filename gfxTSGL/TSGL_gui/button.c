#include "button.h"

static void _event_callback(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {
    switch (event) {
        case tsgl_gui_click:
            self->intData = 1;
            self->animationTarget = 1;
            self->needDraw = true;
            if (self->user_callback != NULL) self->user_callback(self, 1, NULL, self->userArg);
            break;

        case tsgl_gui_drop:
            self->intData = 0;
            if (self->animationState == 1) {
                self->animationTarget = 0;
            }
            self->needDraw = true;
            if (self->user_callback != NULL) self->user_callback(self, 0, NULL, self->userArg);
            break;

        default:
            break;
    }
}

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_buttonData* data = self->data;
    tsgl_pos resize = self->animationState * (TSGL_MATH_MIN(self->math_width, self->math_height) * 0.1);

    TSGL_GUI_DRAW(self, fill, self->math_x + resize, self->math_y + resize, self->math_width - (resize * 2), self->math_height - (resize * 2),
        tsgl_color_raw(tsgl_color_combine(self->animationState, data->color, data->pressedColor), self->colormode)
    );

    if (self->intData == 0 && self->animationState == 1) {
        self->animationTarget = 0;
        self->needDraw = true;
    }
}

tsgl_gui* tsgl_gui_addButton(tsgl_gui* gui, tsgl_color color) {
    tsgl_gui_buttonData* data = calloc(1, sizeof(tsgl_gui_buttonData));
    data->color = color;
    data->pressedColor = tsgl_color_mul(data->color, 0.8);

    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->animationSpeedUpMul = 1.5;
    obj->animationSpeedDownMul = 0.5;
    obj->data = data;
    obj->event_callback = _event_callback;
    obj->draw_callback = _draw_callback;
    obj->fillParentSize = true;
    return obj;
}