#include "button.h"

static void _event_callback(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {
    switch (event) {
        case tsgl_gui_click:
            self->intData = 1;
            self->animationTarget = 1;
            self->needDraw = true;
            break;

        case tsgl_gui_drop:
            self->intData = 0;
            if (self->animationState == 1) {
                self->animationTarget = 0;
            }
            self->needDraw = true;
            break;

        default:
            break;
    }
}

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_buttonData* data = self->data;
    TSGL_GUI_DRAW(self, fill, self->math_x, self->math_y, self->math_width, self->math_height,
        tsgl_color_raw(tsgl_color_combine(self->animationState, data->color, data->pressedColor), self->colormode)
    );

    if (self->intData == 0 && self->animationState == 1) {
        self->animationTarget = 0;
        self->needDraw = true;
    }
}

tsgl_gui* tsgl_gui_addButton(tsgl_gui* gui) {
    tsgl_gui* obj = tsgl_gui_addObject(gui);
    tsgl_gui* text = tsgl_gui_addText(obj);
    tsgl_gui_buttonData* data = calloc(1, sizeof(tsgl_gui_buttonData));
    data->color = TSGL_BLUE;
    data->pressedColor = tsgl_color_mul(data->color, 0.8);
    data->text = text;
    obj->animationSpeedUpMul = 1;
    obj->animationSpeedDownMul = 0.5;
    obj->data = data;
    obj->event_callback = _event_callback;
    obj->draw_callback = _draw_callback;
    return obj;
}

tsgl_gui* tsgl_gui_button_getTextChild(tsgl_gui* self) {
    return self->children[0];
}