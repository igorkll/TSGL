#pragma once
#include "TSGL.h"
#include <stdint.h>

//used to make a push without using transparentColor
#define TSGL_INVALID_RAWCOLOR ((tsgl_rawcolor) {.invalid = true})
#define TSGL_INVALID_COLOR ((tsgl_color) {.invalid = true})

typedef struct {
    bool invalid;
    uint8_t arr[3];
} tsgl_rawcolor;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    bool invalid;
} tsgl_color;

extern const tsgl_color TSGL_WHITE;
extern const tsgl_color TSGL_ORANGE;
extern const tsgl_color TSGL_MAGENTA;
extern const tsgl_color TSGL_YELLOW;
extern const tsgl_color TSGL_GRAY;
extern const tsgl_color TSGL_CYAN;
extern const tsgl_color TSGL_BLUE;
extern const tsgl_color TSGL_GREEN;
extern const tsgl_color TSGL_RED;
extern const tsgl_color TSGL_BLACK;

tsgl_color tsgl_color_pack(uint8_t r, uint8_t g, uint8_t b);
tsgl_color tsgl_color_combine(float v, tsgl_color color1, tsgl_color color2);
tsgl_color tsgl_color_mul(tsgl_color color, float mul);
tsgl_color tsgl_color_hsv(uint8_t hue, uint8_t saturation, uint8_t value);

uint16_t tsgl_color_to565(tsgl_color color);
tsgl_color tsgl_color_from565(uint16_t color);

uint32_t tsgl_color_toHex(tsgl_color color);
tsgl_color tsgl_color_fromHex(uint32_t color);

tsgl_rawcolor tsgl_color_raw(tsgl_color color, tsgl_colormode colormode);
tsgl_color tsgl_color_uraw(tsgl_rawcolor color, tsgl_colormode colormode);

void tsgl_color_444write(size_t rawindex, uint8_t* buffer, tsgl_rawcolor color);
tsgl_rawcolor tsgl_color_444read(size_t rawindex, uint8_t* buffer);