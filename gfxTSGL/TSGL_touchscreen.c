#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include <esp_err.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

esp_err_t tsgl_touchscreen_i2c(tsgl_touchscreen* touchscreen, gpio_num_t sda, gpio_num_t scl, gpio_num_t intr, gpio_num_t rst) {
    _tsgl_touchscreen_capacitive_i2c* ts = malloc(sizeof(_tsgl_touchscreen_capacitive_i2c));
    touchscreen->type = tsgl_touchscreen_capacitive_i2c;
    touchscreen->ts = (void*)ts;

    ts->sda = sda;
    ts->scl = scl;
    ts->intr = intr;
    ts->rst = rst;

    if (intr >= 0) {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask |= 1ULL << intr;
        io_conf.mode = GPIO_MODE_INPUT;
        gpio_config(&io_conf);
    }

    // reset touchscreen
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

    return ESP_OK;
}

void tsgl_touchscreen_free(tsgl_touchscreen* touchscreen) {
    free(touchscreen->ts);
}