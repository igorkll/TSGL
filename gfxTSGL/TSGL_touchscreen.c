#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_i2c.h"
#include <esp_err.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

typedef struct {
    i2c_port_t host;
    uint8_t address;
} ts_i2c;

static uint8_t i2c_readReg(tsgl_touchscreen* touchscreen, uint8_t addr) {
    ts_i2c* ts = (ts_i2c*)touchscreen->ts;
    uint8_t val = 0;
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_write_read_device(ts->host, ts->address, &addr, 1, &val, 1, 100 / portTICK_PERIOD_MS));
    return val;
}

static int i2c_readDualReg(tsgl_touchscreen* touchscreen, uint8_t addr) {
    uint8_t read_buf[2];
    read_buf[0] = i2c_readReg(touchscreen, addr);
    read_buf[1] = i2c_readReg(touchscreen, addr + 1);
    return ((read_buf[0] & 0x0f) << 8) | read_buf[1];
}

esp_err_t tsgl_touchscreen_ft6336u(tsgl_touchscreen* touchscreen, i2c_port_t host, uint8_t address, gpio_num_t intr, gpio_num_t rst) {
    ts_i2c* ts = malloc(sizeof(ts_i2c));
    touchscreen->type = tsgl_touchscreen_capacitive_ft6336u;
    touchscreen->ts = (void*)ts;

    ts->host = host;
    ts->address = address;

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

    return ESP_OK;
}

uint8_t tsgl_touchscreen_touchCount(tsgl_touchscreen* touchscreen) {
    switch (touchscreen->type) {
        case tsgl_touchscreen_capacitive_ft6336u:
            return i2c_readReg(touchscreen, 0x02) & 0x0F;
    }
    return 0;
}

tsgl_touchscreen_point tsgl_touchscreen_getPoint(tsgl_touchscreen* touchscreen, uint8_t index) {
    tsgl_touchscreen_point point = {
        .x = -1,
        .y = -1,
        .z = 0
    };
    switch (touchscreen->type) {
        case tsgl_touchscreen_capacitive_ft6336u:
            switch (index) {
                case 0:
                    point.x = i2c_readDualReg(touchscreen, 0x03);
                    point.y = i2c_readDualReg(touchscreen, 0x05);
                    point.z = 1;
                    break;
                case 1:
                    point.x = i2c_readDualReg(touchscreen, 0x09);
                    point.y = i2c_readDualReg(touchscreen, 0x0B);
                    point.z = 1;
                    break;
            }
            break;
    }
    return point;
}

void tsgl_touchscreen_free(tsgl_touchscreen* touchscreen) {
    free(touchscreen->ts);
}