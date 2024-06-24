#pragma once
#include <driver/i2c.h>
#include <driver/gpio.h>

esp_err_t tsgl_i2c_init(i2c_port_t host, gpio_num_t sda, gpio_num_t scl);