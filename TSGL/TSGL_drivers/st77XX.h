#pragma once
#include "../TSGL.h"
#include "../TSGL_display.h"

extern const tsgl_driver st77XX_rgb444; //does not work on st7796
extern const tsgl_driver st77XX_rgb565; //the most optimal option
extern const tsgl_driver st77XX_rgb666; //3 bytes per pixel. 6 bits are not used
extern const tsgl_driver st77XX_rgb888; //does not work on st7735. on st7789 16M truncated - in fact, it does not differ from st77XX_rgb666. it is recommended to use only with st7796