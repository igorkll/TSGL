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
    tsgl_pos resize = self->animationState * (TSGL_MATH_MIN(self->math_width, self->math_height) * 0.05);
    float percent = 1 - (0.1 * self->animationState);

    tsgl_pos x = self->math_x + resize;
    tsgl_pos y = self->math_y + resize;
    tsgl_pos width = self->math_width - (resize * 2);
    tsgl_pos height = self->math_height - (resize * 2);

    TSGL_GUI_DRAW(self, fill, x, y, width, height,
        tsgl_color_raw(tsgl_color_combine(self->animationState, data->color, data->pressedColor), self->colormode)
    );

    if (self->childrenCount > 0) {
        tsgl_gui* parent = self->children[0];
        parent->math_x = x;
        parent->math_y = y;
        parent->math_width = width;
        parent->math_height = height;
    }

    switch (data->childType) {
        case 1:
            tsgl_gui* parent = self->children[0];
            tsgl_gui_textData* parentData = parent->data;
            parentData->sets.scaleX = percent;
            parentData->sets.scaleY = percent;
            break;
        
        default:
            break;
    }

    if (self->intData == 0 && self->animationState == 1) {
        self->animationTarget = 0;
        self->needDraw = true;
    }
}

static void* _sceneLink(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0 == 0) {
        tsgl_gui_select(userArg);
    }
    return NULL;
}

tsgl_gui* tsgl_gui_addButton(tsgl_gui* gui, tsgl_color color) {
    tsgl_gui_buttonData* data = calloc(1, sizeof(tsgl_gui_buttonData));
    data->color = color;
    data->pressedColor = tsgl_color_mul(data->color, 0.9);

    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->animationSpeedUpMul = 2;
    obj->animationSpeedDownMul = 1;
    obj->data = data;
    obj->event_callback = _event_callback;
    obj->draw_callback = _draw_callback;
    obj->fillSize = true;
    obj->animationBaseDelta = 0.5;
    return obj;
}

void tsgl_gui_button_sceneLink(tsgl_gui* button, tsgl_gui* scene) {
    button->user_callback = _sceneLink;
    button->userArg = scene;
}

void tsgl_gui_button_setEmpty(tsgl_gui* button) {
    if (button->childrenCount > 0) {
        tsgl_gui_free(button->children[0]);
    }

    tsgl_gui_buttonData* data = button->data;
    data->childType = 0;
}

void tsgl_gui_button_setText(tsgl_gui* button, tsgl_color textColor, tsgl_pos targetWidth, const char* text, bool freeText) {
    tsgl_gui_button_setEmpty(button);

    tsgl_gui* child = tsgl_gui_addText(button);
    tsgl_print_settings sets = {
        .fill = TSGL_INVALID_RAWCOLOR,
        .bg = TSGL_INVALID_RAWCOLOR,
        .fg = tsgl_color_raw(textColor, button->colormode),
        .font = tsgl_font_defaultFont,
        .locationMode = tsgl_print_start_top,
        .multiline = true,
        .globalCentering = true,
        .targetWidth = targetWidth
    };

    tsgl_gui_buttonData* data = button->data;
    data->childType = 1;

    tsgl_gui_text_setParams(child, sets);
    tsgl_gui_text_setText(child, text, freeText);
}

void tsgl_gui_button_setRawText(tsgl_gui* button, tsgl_print_settings sets, const char* text, bool freeText) {
    tsgl_gui_button_setEmpty(button);

    tsgl_gui* child = tsgl_gui_addText(button);
    tsgl_gui_buttonData* data = button->data;
    data->childType = 1;

    tsgl_gui_text_setParams(child, sets);
    tsgl_gui_text_setText(child, text, freeText);
}