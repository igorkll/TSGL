#pragma once
#include <driver/gpio.h>

int8_t tsgl_ledc_new(gpio_num_t pin, bool invert);
void tsgl_ledc_set(int8_t channel, bool invert, uint8_t value);