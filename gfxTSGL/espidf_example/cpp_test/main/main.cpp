#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wnarrowing"
#pragma GCC diagnostic push

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <TSGL.hpp>
#include <TSGL_drivers/st7789.h>

#define DC 21
#define CS 22
#define RST 18

tsgl_driver_settings driverSettings = {
    .width = 240,
    .height = 320
};

TSGL_Display display;

extern "C" void app_main() {
    display.begin(&st7789_rgb565, driverSettings, true, TSGL_HOST1, 20000000, DC, CS, RST);

    display.clear(TSGL_GREEN);
    display.update();
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (true) {

    }
}