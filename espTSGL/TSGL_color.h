#pragma once
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} tsgl_color;

tsgl_color tsgl_color_pack(uint8_t r, uint8_t g, uint8_t b);
tsgl_color tsgl_color_combine(float v, tsgl_color color1, tsgl_color color2);

uint16_t tsgl_color_to565(tsgl_color color);
tsgl_color tsgl_color_from565(uint16_t color);

uint32_t tsgl_color_toHex(tsgl_color color);
tsgl_color tsgl_color_fromHex(uint32_t color);

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