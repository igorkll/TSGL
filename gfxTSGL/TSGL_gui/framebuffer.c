#include "framebuffer.h"

static void _draw_callback(tsgl_gui* self) {
    tsgl_gui_framebuffer_settings* settings = self->data;
    TSGL_GUI_DRAW(self, push, self->math_x, self->math_y, settings->rotation, settings->framebuffer, settings->transparentColor);
}

tsgl_gui* tsgl_gui_framebuffer(tsgl_gui* gui, uint8_t rotation, tsgl_framebuffer* framebuffer, tsgl_rawcolor transparentColor) {
    tsgl_gui_framebuffer_settings* settings = malloc(sizeof(tsgl_gui_framebuffer_settings));
    settings->rotation = rotation;
    settings->framebuffer = framebuffer;
    settings->transparentColor = transparentColor;

    tsgl_gui* obj = tsgl_gui_addObject(gui);
    obj->data = (void*)settings;
    obj->draw_callback = _draw_callback;
    return obj;
}