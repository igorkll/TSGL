#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef int16_t tsgl_pos;

extern const uint8_t tsgl_colormodeSizes[];

typedef enum {
    tsgl_rgb565_le,
    tsgl_rgb565_be,
    tsgl_bgr565_le,
    tsgl_bgr565_be,
    tsgl_rgb888,
    tsgl_bgr888,
} tsgl_colormode;

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes;
    int16_t delay; //-1 = end of commands
} tsgl_driver_command;

typedef struct {
    tsgl_driver_command init[64];
    tsgl_driver_command enable[8];
    tsgl_driver_command disable[8];
} tsgl_driver;