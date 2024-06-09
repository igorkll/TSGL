#include "../TSGL.h"
#include <stdint.h>

static tsgl_driver_list _st7789_select(tsgl_pos x, tsgl_pos y, tsgl_pos x2, tsgl_pos y2) {
    tsgl_driver_list list = {
        .list = {
            {0x2A, {x >> 8, x & 0xff, x2 >> 8, x2 & 0xff}, 4},
            {0x2B, {y >> 8, y & 0xff, y2 >> 8, y2 & 0xff}, 4},
            {0x2C, {}, 0, -1}
        }
    };
    return list;
}

static tsgl_driver_list _st7789_rotate(uint8_t rotation) {
    switch (rotation) {
        default : {
            tsgl_driver_list list = {
                .list = {
                    {0x36, {(1<<5) | (1<<6)}, 1, -1}
                }
            };
            return list;
        }

        case 1 : {
            tsgl_driver_list list = {
                .list = {
                    {0x36, {(1<<5)}, 1, -1}
                }
            };
            return list;
        }

        case 2 : {
            tsgl_driver_list list = {
                .list = {
                    {0x36, {(1<<5) | (1<<7)}, 1, -1}
                }
            };
            return list;
        }

        case 3 : {
            tsgl_driver_list list = {
                .list = {
                    {0x36, {(1<<5) | (1<<6) | (1<<7)}, 1, -1}
                }
            };
            return list;
        }
    }
}

static tsgl_driver_list _st7789_invert(bool invert) {
    if (invert) {
        tsgl_driver_list list = {
            .list = {
                {0x21, {}, 0, -1}
            }
        };
        return list;
    } else {
        tsgl_driver_list list = {
            .list = {
                {0x20, {}, 0, -1}
            }
        };
        return list;
    }
}

#define _ST7789_SERVICE_CODE \
    /* Memory Data Access Control */ \
    {0x36, {(1<<5)|(1<<6)}, 1}, \
    /* Porch Setting */ \
    {0xB2, {0x0c, 0x0c, 0x00, 0x33, 0x33}, 5}, \
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */ \
    {0xB7, {0x45}, 1}, \
    /* VCOM Setting, VCOM=1.175V */ \
    {0xBB, {0x2B}, 1}, \
    /* LCM Control, XOR: BGR, MX, MH */ \
    {0xC0, {0x2C}, 1}, \
    /* VDV and VRH Command Enable, enable=1 */ \
    {0xC2, {0x01, 0xff}, 2}, \
    /* VRH Set, Vap=4.4+... */ \
    {0xC3, {0x11}, 1}, \
    /* VDV Set, VDV=0 */ \
    {0xC4, {0x20}, 1}, \
    /* Frame Rate Control, 60Hz, inversion=0 */ \
    {0xC6, {0x0f}, 1}, \
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */ \
    {0xD0, {0xA4, 0xA1}, 1}, \
    /* Positive Voltage Gamma Control */ \
    {0xE0, {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19}, 14}, \
    /* Negative Voltage Gamma Control */ \
    {0xE1, {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19}, 14, -1} \
}, \
.enable = { \
    {0x11, {0}, 0, 100}, \
    {0x38, {0}, 0, 0}, \
    {0x29, {0}, 0, -1} \
}, \
.disable = { \
    {0x28, {0}, 0, 0}, \
    {0x39, {0}, 0, 0}, \
    {0x10, {0}, 0, -1} \
}, \
.select = _st7789_select, \
.rotate = _st7789_rotate, \
.invert = _st7789_invert

static const tsgl_driver st7789_rgb444 = {
    .colormode = tsgl_rgb444,
    .init = {
        {0x3A, {0x03}, 1}, //444
    _ST7789_SERVICE_CODE
};

static const tsgl_driver st7789_rgb565 = {
    .colormode = tsgl_rgb565_be,
    .init = {
        {0x3A, {0x05}, 1}, //565
    _ST7789_SERVICE_CODE
};

static const tsgl_driver st7789_rgb666 = { //3 bytes per pixel. 6 bits are not used
    .colormode = tsgl_rgb888,
    .init = {
        {0x3A, {0x06}, 1}, //666
    _ST7789_SERVICE_CODE
};

static const tsgl_driver st7789_rgb888 = {
    .colormode = tsgl_rgb888,
    .init = {
        {0x3A, {0x07}, 1}, //16M truncated - truncated true color support. that's what it says in the documentation
    _ST7789_SERVICE_CODE
};