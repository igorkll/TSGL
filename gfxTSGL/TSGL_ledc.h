#pragma once
#include "TSGL.h"
#include <driver/gpio.h>

typedef struct {
    int8_t channel;
    bool invert;
} tsgl_ledc;

int8_t tsgl_ledc_getChannel();
int8_t tsgl_ledc_getFastChannel();
uint8_t tsgl_ledc_CRTValue(uint8_t val);

esp_err_t tsgl_ledc_new(tsgl_ledc* obj, gpio_num_t pin, bool invert, uint8_t defaultValue);
esp_err_t tsgl_ledc_newFast(tsgl_ledc* obj, gpio_num_t pin, bool invert, uint8_t defaultValue);
void tsgl_ledc_set(tsgl_ledc* obj, uint8_t value);
void tsgl_ledc_rawSet(tsgl_ledc* obj, uint8_t value);
void tsgl_ledc_free(tsgl_ledc* obj);