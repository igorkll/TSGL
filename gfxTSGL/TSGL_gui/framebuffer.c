#include "framebuffer.h"

static void _draw_callback(tsgl_gui* self) {
    TSGL_GUI_DRAW(self, push, self->math_x, self->math_y, (tsgl_sprite*)self->data);
}

tsgl_gui* tsgl_gui_addFramebuffer(tsgl_gui* gui, tsgl_sprite* sprite) {
    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->data = sprite;
    obj->draw_callback = _draw_callback;
    obj->noFreeData = true;
    return obj;
}