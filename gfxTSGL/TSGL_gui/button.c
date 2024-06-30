#include "../TSGL.h"
#include "../TSGL_gui.h"

static void _event_callback(tsgl_gui* self, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {
    switch (event) {
        case tsgl_gui_click:
            self->intData = 1;
            break;

        case tsgl_gui_drop:
            self->intData = 0;
            break;

        default:
            break;
    }
    self->needDraw = true;
}

static void _draw_callback(tsgl_gui* self) {
    if (self->buffered) {
        tsgl_framebuffer_fill(self->target, self->math_x, self->math_y, self->math_width, self->math_height,
            tsgl_color_raw(tsgl_color_fromHex(self->intData ? 0x29d667 : 0x29d6cc), self->colormode)
        );
    } else {
        tsgl_display_fill(self->target, self->math_x, self->math_y, self->math_width, self->math_height,
            tsgl_color_raw(tsgl_color_fromHex(self->intData ? 0x29d667 : 0x29d6cc), self->colormode)
        );
    }
}

tsgl_gui* tsgl_gui_addButton(tsgl_gui* gui) {
    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->draw_callback = _draw_callback;
    obj->event_callback = _event_callback;
    return obj;
}