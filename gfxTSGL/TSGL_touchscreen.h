#pragma once
#include "TSGL.h"
#include "TSGL_i2c.h"
#include <esp_err.h>
#include <driver/gpio.h>

typedef enum {
    tsgl_touchscreen_capacitive_ft6336u
} tsgl_touchscreen_type;

typedef struct {
    tsgl_pos x;
    tsgl_pos y;
    float z;
} tsgl_touchscreen_point;

typedef struct {
    tsgl_touchscreen_type type;
    void* ts;

    uint8_t rotation;
    uint8_t count;
    tsgl_touchscreen_point touch[10];
} tsgl_touchscreen;

esp_err_t tsgl_touchscreen_ft6336u(tsgl_touchscreen* touchscreen, i2c_port_t host, uint8_t address, gpio_num_t intr, gpio_num_t rst);
void tsgl_touchscreen_read(tsgl_touchscreen* touchscreen);
void tsgl_touchscreen_free(tsgl_touchscreen* touchscreen);