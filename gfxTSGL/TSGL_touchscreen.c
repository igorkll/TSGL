#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_i2c.h"
#include <esp_err.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

esp_err_t tsgl_touchscreen_i2c(tsgl_touchscreen* touchscreen, i2c_port_t host, uint8_t address, gpio_num_t intr, gpio_num_t rst) {
    _tsgl_touchscreen_capacitive_i2c* ts = malloc(sizeof(_tsgl_touchscreen_capacitive_i2c));
    touchscreen->type = tsgl_touchscreen_capacitive_i2c;
    touchscreen->ts = (void*)ts;
    ts->intr = intr;
    ts->address = address;
    ts->host = host;

    if (intr >= 0) {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask |= 1ULL << intr;
        io_conf.mode = GPIO_MODE_INPUT;
        gpio_config(&io_conf);
    }

    if (rst >= 0) {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask |= 1ULL << rst;
        io_conf.mode = GPIO_MODE_OUTPUT;
        gpio_config(&io_conf);

        gpio_set_level(rst, false);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(rst, true);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 100000,
    };

    esp_err_t result = i2c_master_bus_add_device(bus_handle, &dev_cfg, &ts.dev_handle);
    if (result == ESP_OK) {
        
    }
    return result;
}

void tsgl_touchscreen_free(tsgl_touchscreen* touchscreen) {
    free(touchscreen->ts);
}