#pragma once
#include <driver/i2c_master.h>

esp_err_t tsgl_i2c_init(i2c_master_bus_handle_t** bus_handle, i2c_port_t host, gpio_num_t sda, gpio_num_t scl);
esp_err_t tsgl_i2c_free(i2c_master_bus_handle_t* bus_handle);