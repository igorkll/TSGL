#include <TSGL.hpp>
#include <TSGL_drivers/st7789.h>

tsgl_driver_settings driverSettings = {
    .width = 240,
    .height = 320
};

TSGL_Display display(&st7789_rgb565, driverSettings, true, );

void setup() {
  
}

void loop() {
    display.clear(TSGL_GREEN);
    display.update();
}