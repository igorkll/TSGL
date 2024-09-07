#include "system.h"
#include <TSGL_gui.h>
#include <TSGL_gui/button.h>
#include <TSGL_gui/sprite.h>
#include <TSGL_gui/colorpicker.h>

static tsgl_gui* gui;
static tsgl_gui* mainScene;
static tsgl_gui* plate_up;
static tsgl_gui* button_powerOff;

static void* callback_powerOff(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0 == 0) {
        system_powerOff();
    }
    return NULL;
}

static void gui_loop() {
    while (true) {
        tsgl_keyboard_readAll(&keyboard);
        tsgl_gui_processTouchscreen(gui, &touchscreen);
        tsgl_gui_processGui(gui, NULL, &benchmark, 0);
        tsgl_benchmark_printRealFPS(&benchmark);
    }
}

static void gui_scene_main() {
    mainScene = tsgl_gui_addObject(gui);
    mainScene->color = tsgl_color_raw(tsgl_color_fromHex(0x018db4), gui->colormode);

    tsgl_gui_addColorpicker(mainScene, (tsgl_gui_colorpickerConfig) {});

    plate_up = tsgl_gui_addObject(mainScene);
    plate_up->x = 0;
    plate_up->y = 0;
    plate_up->width = framebuffer.width;
    plate_up->height = 50;
    plate_up->color = tsgl_color_raw(tsgl_color_fromHex(0x757575), plate_up->colormode);
    plate_up->draggable = true;

    button_powerOff = tsgl_gui_addButton(plate_up);
    button_powerOff->x = 0;
    button_powerOff->y = 0;
    button_powerOff->width = 150;
    button_powerOff->user_callback = callback_powerOff;
    tsgl_gui_button_setStyle(button_powerOff, TSGL_RED, tsgl_color_fromHex(0x9c6800), tsgl_gui_button_fill);
    tsgl_gui_button_setText(button_powerOff, TSGL_WHITE, 8, "power off", false);

    tsgl_gui_select(mainScene);
}

void gui_init() {
    gui = tsgl_gui_createRoot_buffer(&display, &framebuffer);
    //gui = tsgl_gui_createRoot_display(&display, display.colormode);
    gui_scene_main();
    xTaskCreate(gui_loop, NULL, 4096, NULL, 24, NULL);
}