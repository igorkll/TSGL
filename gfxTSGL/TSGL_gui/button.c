#include "../TSGL.h"
#include "../TSGL_gui.h"

static void _draw_callback(tsgl_gui* self) {
    tsgl_framebuffer_fill(self->target, self->math_x, self->math_y, self->math_width, self->math_height, tsgl_color_raw(tsgl_color_fromHex(0x29d6cc), self->colormode));
}

tsgl_gui* tsgl_gui_addButton(tsgl_gui* gui) {
    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->draw_callback = _draw_callback;
    return obj;
}