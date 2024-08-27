#include "sprite.h"

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_spriteData* data = self->data;
    if (data->autoscale) {
        data->sprite->resizeWidth = self->math_width;
        data->sprite->resizeHeight = self->math_height;
    }
    TSGL_GUI_DRAW(self, push, self->math_x, self->math_y, data->sprite);
}

tsgl_gui* tsgl_gui_addSprite(tsgl_gui* gui, tsgl_sprite* sprite, bool autoscale) {
    tsgl_gui* obj = tsgl_gui_addObject(gui);
    tsgl_gui_spriteData* data = malloc(sizeof(tsgl_gui_spriteData));
    data->sprite = sprite;
    data->autoscale = autoscale;
    obj->data = data;
    obj->draw_callback = _draw_callback;
    return obj;
}

void tsgl_gui_sprite_setParams(tsgl_gui* self, tsgl_sprite* sprite, bool autoscale) {
    tsgl_gui_spriteData* data = self->data;
    data->sprite = sprite;
    data->autoscale = autoscale;
}