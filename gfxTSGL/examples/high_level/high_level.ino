#include <TSGL.hpp>
#include <TSGL_drivers/st7789.h>

tsgl_driver_settings driverSettings = {
    .width = 240,
    .height = 320
};
#define DC 21 
#define CS 22
#define RST 18
TSGL_Display display(&st7789_rgb565, driverSettings, true, TSGL_HOST1, 20000000, DC, CS, RST);

void setup() {
  
}

void loop() {
    display.clear(TSGL_GREEN);
    display.update();
}