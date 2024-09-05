#include "system.h"

static tsgl_gui* gui;
static tsgl_gui* mainScene;

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
        tsgl_gui_processGui(gui, &framebuffer2, &benchmark, portTICK_PERIOD_MS);
        //tsgl_benchmark_print(&benchmark);
        vTaskDelay(1);
    }
}

static void gui_scene_main() {
    mainScene = tsgl_gui_addObject(gui);
    mainScene->color = tsgl_color_raw(tsgl_color_fromHex(0x018db4), gui->colormode);

    tsgl_gui_select(mainScene);
}

void gui_init() {
    gui = tsgl_gui_createRoot_buffer(&display, &framebuffer);
    gui_scene_main();
    xTaskCreate(gui_loop, NULL, 4096, NULL, tskIDLE_PRIORITY, NULL);
}