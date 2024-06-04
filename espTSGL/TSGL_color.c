#include "TSGL.h"
#include "TSGL_color.h"

tsgl_color tsgl_color_pack(uint8_t r, uint8_t g, uint8_t b) {
    tsgl_color color = {
        .r = r,
        .g = g,
        .b = b
    };
    return color;
}