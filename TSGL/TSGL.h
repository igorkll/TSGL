#pragma once
#include <esp_heap_caps.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <driver/gpio.h>

typedef int16_t tsgl_pos;
#define TSGL_POS_MIN -32768
#define TSGL_POS_MAX 32767

#define TSGL_SPIRAM   MALLOC_CAP_SPIRAM
#define TSGL_RAM      0

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
    tsgl_monochrome
} tsgl_colormode;

size_t tsgl_getPartSize();
void tsgl_sendFlood(size_t maxPart, void* arg, bool(*send)(void* arg, void* part, size_t size), const uint8_t* data, size_t size, size_t flood);
void* tsgl_malloc(size_t size, int64_t caps);
void tsgl_delay(size_t time);

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
    tsgl_driver_list (*backlight) (const tsgl_driver_storage* storage, uint8_t value);
} tsgl_driver;

#include "TSGL_color.h"

typedef enum {
    tsgl_display_init_none = 0,
    tsgl_display_init_color,
    tsgl_display_init_framebuffer
} tsgl_display_init;

typedef struct {
    const tsgl_driver* driver;
    bool invertBacklight;
    bool invert;
    bool swapRGB;
    bool flipX;
    bool flipY;
    bool flipXY;
    tsgl_pos width;
    tsgl_pos height;
    tsgl_pos offsetX; //on many displays, the visible area does not start from the beginning
    tsgl_pos offsetY;

    // the first state after initialization
    tsgl_display_init init_state;
    tsgl_rawcolor init_color;

    const uint8_t* init_framebuffer_ptr;
    size_t init_framebuffer_size;
    uint8_t init_framebuffer_rotation;

    bool backlight_init;
    gpio_num_t backlight_pin;
    uint8_t backlight_value;
} tsgl_display_settings;

// ----------------

#include "TSGL_math.h"

#define TSGL_SET_REFERENCE(name) void(*name)(void* arg, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color)
#define TSGL_FILL_REFERENCE(name) void(*name)(void* arg, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color)

typedef struct tsgl_sprite tsgl_sprite;