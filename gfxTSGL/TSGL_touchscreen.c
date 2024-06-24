#pragma once
#include "TSGL.h"
#include <esp_err.h>

typedef enum {
    tsgl_touchscreen_capacitive
} tsgl_touchscreen_type;

typedef struct {

} tsgl_touchscreen_capacitive;

typedef struct {
    tsgl_touchscreen_type type;
    void* touchscreen;
} tsgl_touchscreen;

typedef struct {
    tsgl_pos x;
    tsgl_pos y;
    float z;
} tsgl_touchscreen_point;

esp_err_t tsgl_touchscreen_initCapacitive(tsgl_touchscreen_point* touchscreen);
uint8_t tsgl_touchscreen_getTouchCount(tsgl_touchscreen_point* touchscreen);
tsgl_touchscreen_point tsgl_touchscreen_getPoint(tsgl_touchscreen_point* touchscreen, uint8_t i);