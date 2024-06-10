#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wnarrowing"
#pragma GCC diagnostic push

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <TSGL.hpp>
#include <TSGL_drivers/st7789.h>

tsgl_driver_settings driverSettings = {
    .width = 240,
    .height = 320
};
#define DC 21 
#define CS 22
#define RST 18
TSGL_Display display(&st7789_rgb565, driverSettings, false, TSGL_HOST1, 20000000, DC, CS, RST);

extern "C" void app_main() {
    display.clear(TSGL_GREEN);
    display.update();

    tsgl_display_clear(&display.display, tsgl_color_raw(TSGL_RED, display.display.colormode));

    while (true) {
        printf("loop\n")
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}