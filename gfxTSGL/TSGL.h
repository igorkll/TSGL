#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>

typedef int16_t tsgl_pos;

extern const float tsgl_colormodeSizes[];

typedef enum {
    tsgl_rgb565_le = 0,
    tsgl_bgr565_le,
    tsgl_rgb565_be,
    tsgl_bgr565_be,
    tsgl_rgb888,
    tsgl_bgr888,
    tsgl_rgb444,
    tsgl_bgr444,
} tsgl_colormode;

size_t tsgl_getPartSize();
void tsgl_sendFlood(void* arg, void(*send)(void* arg, void* part, size_t size), const uint8_t* data, size_t size, size_t flood);

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
    size_t list[16];
    bool swapRGB;
    bool flipX;
    bool flipY;
    bool flipXY;
} tsgl_driver_storage;

typedef struct {
    bool selectAreaAfterCommand;
    tsgl_colormode colormode;
    tsgl_driver_command init[64];
    tsgl_driver_storage storage;
    tsgl_driver_list (*enable) (const tsgl_driver_storage* storage, bool state);
    tsgl_driver_list (*select) (const tsgl_driver_storage* storage, tsgl_pos x, tsgl_pos y, tsgl_pos x2, tsgl_pos y2);
    tsgl_driver_list (*rotate) (const tsgl_driver_storage* storage, uint8_t rotation);
    tsgl_driver_list (*invert) (const tsgl_driver_storage* storage, bool invert);
} tsgl_driver;

// ---------------- settings

typedef struct {
    const tsgl_driver* driver;
    bool invert;
    bool swapRGB;
    bool flipX;
    bool flipY;
    bool flipXY;
    tsgl_pos width;
    tsgl_pos height;
    tsgl_pos offsetX; //on many displays, the visible area does not start from the beginning
    tsgl_pos offsetY;
} tsgl_settings;