#include "../TSGL.h"
#include "../TSGL_ledc.h"

#define _ROTATION_0 0
#define _ROTATION_1 (1<<5) | (1<<6) | (1<<2)
#define _ROTATION_2 (1<<6) | (1<<7) | (1<<2) | (1<<4)
#define _ROTATION_3 (1<<5) | (1<<7) | (1<<4)

static tsgl_driver_list _select(const tsgl_driver_storage* storage, tsgl_pos x, tsgl_pos y, tsgl_pos x2, tsgl_pos y2) {
    tsgl_driver_list list = {
        .list = {
            {0x2A, {x >> 8, x & 0xff, x2 >> 8, x2 & 0xff}, 4},
            {0x2B, {y >> 8, y & 0xff, y2 >> 8, y2 & 0xff}, 4},
            {0x2C, {0}, 0, -1}
        }
    };
    return list;
}

static tsgl_driver_list _rotate(const tsgl_driver_storage* storage, uint8_t rotation) {
    uint8_t regvalue = 0;
    switch (rotation) {
        default:
            regvalue = _ROTATION_0;
            break;

        case 1:
            regvalue = _ROTATION_1;
            break;

        case 2:
            regvalue = _ROTATION_2;
            break;

        case 3:
            regvalue = _ROTATION_3;
            break;
    }

    if (storage->swapRGB) regvalue ^= (1 << 3);
    if (storage->flipX) {
        regvalue ^= (1 << 6);
        regvalue ^= (1 << 2);
    }
    if (storage->flipY) {
        regvalue ^= (1 << 7);
        regvalue ^= (1 << 4);
    }
    if (storage->flipXY) {
        regvalue ^= (1 << 5);
    }

    tsgl_driver_list list = {
        .list = {
            {0x36, {regvalue}, 1, -1}
        }
    };
    return list;
}

static tsgl_driver_list _invert(const tsgl_driver_storage* storage, bool invert) {
    if (invert) {
        tsgl_driver_list list = {
            .list = {
                {0x21, {0}, 0, -1}
            }
        };
        return list;
    } else {
        tsgl_driver_list list = {
            .list = {
                {0x20, {0}, 0, -1}
            }
        };
        return list;
    }
}

static tsgl_driver_list _enable(const tsgl_driver_storage* storage, bool state) {
    if (state) {
        tsgl_driver_list list = {
            .list = {
                {0x11, {0}, 0, 100}, //Sleep Out
                {0x38, {0}, 0, 0}, //Idle mode off
                {0x29, {0}, 0, -1} //Display On
            }
        };
        return list;
    } else {
        tsgl_driver_list list = {
            .list = {
                {0x28, {0}, 0, 0}, //Display Off
                {0x39, {0}, 0, 0}, //Idle mode on
                {0x10, {0}, 0, -1} //Sleep In
            }
        };
        return list;
    }
}

static tsgl_driver_list _backlight(const tsgl_driver_storage* storage, uint8_t value) {
    tsgl_driver_list list = {
        .list = {
            {0x53, {0b00101000}, 1, 0},
            {0x51, {tsgl_ledc_CRTValue(value)}, 1, -1}
        }
    };
    return list;
}

#define _SERVICE_CODE \
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
    /* Frame Rate Control, 111Hz, inversion=0. if your screen does not work stably at such a refresh rate, then set the 0x0f mode, which will be equal to 60 hertz */ \
    {0xC6, {0x01}, 1}, \
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */ \
    {0xD0, {0xA4, 0xA1}, 1}, \
    /* Positive Voltage Gamma Control */ \
    {0xE0, {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19}, 14}, \
    /* Negative Voltage Gamma Control */ \
    {0xE1, {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19}, 14}, \
    /* Sleep Out */ \
    {0x11, {0}, 0, 100}, \
    /* Idle mode off */ \
    {0x38, {0}, 0, -1} \
}, \
.enable = _enable, \
.select = _select, \
.rotate = _rotate, \
.invert = _invert, \
.backlight = _backlight, \
.selectAreaAfterCommand = true



const tsgl_driver st77XX_rgb444 = { //does not work on st7796
    .colormode = tsgl_rgb444,
    .init = {
        {0x3A, {0x03}, 1}, //444
    _SERVICE_CODE
};

const tsgl_driver st77XX_rgb565 = {
    .colormode = tsgl_rgb565_le,
    .init = {
        {0x3A, {0x05}, 1}, //565
        {0xB0, {0b00000000, 0b00001000}, 2},
    _SERVICE_CODE
};

const tsgl_driver st77XX_rgb666 = { //3 bytes per pixel. 6 bits are not used
    .colormode = tsgl_rgb888,
    .init = {
        {0x3A, {0x06}, 1}, //666
    _SERVICE_CODE
};

const tsgl_driver st77XX_rgb888 = { //does not work on st7735
    .colormode = tsgl_rgb888,
    .init = {
        {0x3A, {0x07}, 1}, //on st7789 16M truncated - truncated true color support. that's what it says in the documentation
    _SERVICE_CODE
};