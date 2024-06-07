#include "../TSGL.h"

static const tsgl_driver st7789_rgb565 = {
    .init = {
        /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
        {0x36, {(1<<5)|(1<<6)}, 1},
        /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
        {0x3A, {0x05}, 1}, //0x05 / 0x06
        /* Porch Setting */
        {0xB2, {0x0c, 0x0c, 0x00, 0x33, 0x33}, 5},
        /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
        {0xB7, {0x45}, 1},
        /* VCOM Setting, VCOM=1.175V */
        {0xBB, {0x2B}, 1},
        /* LCM Control, XOR: BGR, MX, MH */
        {0xC0, {0x2C}, 1},
        /* VDV and VRH Command Enable, enable=1 */
        {0xC2, {0x01, 0xff}, 2},
        /* VRH Set, Vap=4.4+... */
        {0xC3, {0x11}, 1},
        /* VDV Set, VDV=0 */
        {0xC4, {0x20}, 1},
        /* Frame Rate Control, 60Hz, inversion=0 */
        {0xC6, {0x0f}, 1},
        /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
        {0xD0, {0xA4, 0xA1}, 1},
        /* Positive Voltage Gamma Control */
        {0xE0, {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19}, 14},
        /* Negative Voltage Gamma Control */
        {0xE1, {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19}, 14},
        /* selecting a rendering area */
        {0x2A, {0, 0, (320)>>8, (320)&0xff}, 4},
        {0x2B, {0>>8, 0&0xff, 320>>8, 320&0xff}, 4},
        {0x2C, {}, 0},
        /* Sleep Out */
        {0x11, {0}, 0, -1}
    },
    .enable = {
        {0x29, {0}, 0, 0}
    },
    .disable = {
        {0x28, {0}, 0, 0}
    }
};