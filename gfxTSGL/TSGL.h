#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// ---------------- base

typedef int16_t tsgl_pos;

#define TSGL_GFX_RECT(arg, fill, x, y, width, height, color, strokelen) \
fill(arg, x, y, width, strokelen, color); \
fill(arg, x, (y + height) - strokelen, width, strokelen, color); \
fill(arg, x, y + 1, strokelen, height - 2, color); \
fill(arg, (x + width) - strokelen, y + 1, strokelen, height - 2, color)

// ---------------- color

extern const float tsgl_colormodeSizes[];
typedef enum {
    tsgl_rgb565_le,
    tsgl_rgb565_be,
    tsgl_bgr565_le,
    tsgl_bgr565_be,
    tsgl_rgb888,
    tsgl_bgr888,
    tsgl_rgb444,
    tsgl_bgr444,
} tsgl_colormode;

// ---------------- driver

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t datalen;
    int16_t delay; //-1 = end of commands
} tsgl_driver_command;

typedef struct {
    tsgl_driver_command list[8];
} tsgl_driver_list;

typedef struct {
    tsgl_colormode colormode;
    tsgl_driver_command init[64];
    tsgl_driver_command enable[8];
    tsgl_driver_command disable[8];
    tsgl_driver_list (*select) (tsgl_pos x, tsgl_pos y, tsgl_pos x2, tsgl_pos y2);
    tsgl_driver_list (*rotate) (uint8_t rotation);
    tsgl_driver_list (*invert) (bool invert);
} tsgl_driver;

// ---------------- settings

typedef struct {
    bool invert;
    tsgl_pos width;
    tsgl_pos height;
    tsgl_pos offsetX;
    tsgl_pos offsetY;
} tsgl_driver_settings;