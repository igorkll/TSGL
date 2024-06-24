#pragma once
#include "TSGL.h"
#include "TSGL_i2c.h"
#include <esp_err.h>
#include <driver/gpio.h>

typedef enum {
    tsgl_touchscreen_capacitive_i2c
} tsgl_touchscreen_type;

typedef struct {
    tsgl_pos x;
    tsgl_pos y;
    float z;
} tsgl_touchscreen_point;

typedef struct {
    tsgl_touchscreen_type type;
    void* ts;
    uint8_t touchCount;
    tsgl_touchscreen_point touchs[10];
} tsgl_touchscreen;

esp_err_t tsgl_touchscreen_i2c(tsgl_touchscreen* touchscreen, i2c_port_t host, uint8_t address, gpio_num_t intr, gpio_num_t rst);
void tsgl_touchscreen_read(tsgl_touchscreen* touchscreen);
uint8_t tsgl_touchscreen_getTouchCount(tsgl_touchscreen* touchscreen);
tsgl_touchscreen_point tsgl_touchscreen_getPoint(tsgl_touchscreen* touchscreen, uint8_t i);
void tsgl_touchscreen_free(tsgl_touchscreen* touchscreen);