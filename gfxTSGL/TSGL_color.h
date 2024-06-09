#pragma once
#include <TSGL.h>
#include <stdint.h>

typedef struct {
    uint8_t arr[3];
} tsgl_rawcolor;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} tsgl_color;

#define TSGL_WHITE tsgl_color_fromHex(0xffffff)
#define TSGL_ORANGE tsgl_color_fromHex(0xF2B233)
#define TSGL_MAGENTA tsgl_color_fromHex(0xE57FD8)
#define TSGL_LIGHT_BLUE tsgl_color_fromHex(0x99B2F2)
#define TSGL_YELLOW tsgl_color_fromHex(0xDEDE6C)
#define TSGL_LIME tsgl_color_fromHex(0x7FCC19)
#define TSGL_PINK tsgl_color_fromHex(0xF2B2CC)
#define TSGL_GRAY tsgl_color_fromHex(0x4C4C4C)
#define TSGL_LIGHT_GRAY tsgl_color_fromHex(0x999999)
#define TSGL_CYAN tsgl_color_fromHex(0x4C99B2)
#define TSGL_PURPLE tsgl_color_fromHex(0xB266E5)
#define TSGL_BLUE tsgl_color_fromHex(0x3366CC)
#define TSGL_BROWN tsgl_color_fromHex(0x7F664C)
#define TSGL_GREEN tsgl_color_fromHex(0x57A64E)
#define TSGL_RED tsgl_color_fromHex(0xCC4C4C)
#define TSGL_BLACK tsgl_color_fromHex(0x191919)

tsgl_color tsgl_color_pack(uint8_t r, uint8_t g, uint8_t b);
tsgl_color tsgl_color_combine(float v, tsgl_color color1, tsgl_color color2);
tsgl_color tsgl_color_hsv(uint8_t hue, uint8_t saturation, uint8_t value);

uint16_t tsgl_color_to565(tsgl_color color);
tsgl_color tsgl_color_from565(uint16_t color);

uint32_t tsgl_color_toHex(tsgl_color color);
tsgl_color tsgl_color_fromHex(uint32_t color);

tsgl_rawcolor tsgl_color_raw(tsgl_color color, tsgl_colormode colormode);
tsgl_color tsgl_color_uraw(tsgl_rawcolor color, tsgl_colormode colormode);

void tsgl_color_444write(size_t rawindex, uint8_t* buffer, tsgl_rawcolor color);
tsgl_rawcolor tsgl_color_444read(size_t rawindex, uint8_t* buffer);

tsgl_rawcolor tsgl_color_make444(tsgl_rawcolor color);
tsgl_rawcolor tsgl_color_parse444_1(tsgl_rawcolor color);
tsgl_rawcolor tsgl_color_parse444_2(tsgl_rawcolor color);