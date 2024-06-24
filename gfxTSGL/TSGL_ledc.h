#pragma once
#include "TSGL.h"
#include <driver/gpio.h>

uint8_t tsgl_ledc_CRTValue(uint8_t val);
int8_t tsgl_ledc_new(gpio_num_t pin, bool invert, uint8_t value);
void tsgl_ledc_set(int8_t channel, bool invert, uint8_t value);