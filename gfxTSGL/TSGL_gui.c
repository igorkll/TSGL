#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"
#include "TSGL_gui.h"

tsgl_gui* tsgl_gui_createForDisplay(tsgl_display* display) {
    tsgl_gui* gui = malloc(sizeof(tsgl_display));
    gui->containers = NULL;
    gui->target = display;
    gui->buffered = false;
    return gui;
}

tsgl_gui* tsgl_gui_createForBuffer(tsgl_framebuffer* framebuffer) {
    tsgl_gui* gui = malloc(sizeof(tsgl_framebuffer));
    gui->containers = NULL;
    gui->target = framebuffer;
    gui->buffered = true;
    return gui;
}

tsgl_gui_container* tsgl_gui_addContainer(tsgl_gui* gui, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
    return NULL;
}

void tsgl_gui_freeContainer(tsgl_gui_container* container) {

}

void tsgl_gui_free(tsgl_gui* gui) {
    free(gui);
}