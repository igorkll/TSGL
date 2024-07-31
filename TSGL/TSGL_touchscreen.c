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
    //ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_write_read_device(ts->host, ts->address, &addr, 1, &val, 1, 100 / portTICK_PERIOD_MS));
    i2c_master_write_read_device(ts->host, ts->address, &addr, 1, &val, 1, 100 / portTICK_PERIOD_MS); //error handling is disabled because the touchscreen does not always respond
    return val;
}

static int i2c_readDualReg(tsgl_touchscreen* touchscreen, uint8_t addr) {
    uint8_t read_buf[2];
    read_buf[0] = i2c_readReg(touchscreen, addr);
    read_buf[1] = i2c_readReg(touchscreen, addr + 1);
    return ((read_buf[0] & 0x0f) << 8) | read_buf[1];
}

esp_err_t tsgl_touchscreen_ft6336u(tsgl_touchscreen* touchscreen, i2c_port_t host, uint8_t address, gpio_num_t rst) {
    ts_i2c* ts = malloc(sizeof(ts_i2c));
    touchscreen->type = tsgl_touchscreen_capacitive_ft6336u;
    touchscreen->ts = (void*)ts;

    ts->host = host;
    ts->address = address;

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
    float x = 0;
    float y = 0;
    float z = 0;

    switch (touchscreen->type) {
        case tsgl_touchscreen_capacitive_ft6336u:
            switch (index) {
                case 0:
                    x = i2c_readDualReg(touchscreen, 0x03);
                    y = i2c_readDualReg(touchscreen, 0x05);
                    z = 1;
                    break;
                case 1:
                    x = i2c_readDualReg(touchscreen, 0x09);
                    y = i2c_readDualReg(touchscreen, 0x0B);
                    z = 1;
                    break;
            }
            break;
    }

    if (touchscreen->flipXY) {
        tsgl_pos t = x;
        x = y;
        y = t;
    }

    if (touchscreen->mulX != 0) x *= touchscreen->mulX;
    if (touchscreen->mulY != 0) y *= touchscreen->mulY;

    x += touchscreen->offsetX;
    y += touchscreen->offsetY;

    if (x < 0) {
        x = 0;
    } else if (x >= touchscreen->width) {
        x = touchscreen->width - 1;
    }

    if (y < 0) {
        y = 0;
    } else if (y >= touchscreen->height) {
        y = touchscreen->height - 1;
    }

    bool flipFlip = touchscreen->rotation == 2 || touchscreen->rotation == 3;
    if (touchscreen->flipX ^ flipFlip) x = touchscreen->width - 1 - x;
    if (touchscreen->flipY ^ flipFlip) y = touchscreen->height - 1 - y;

    switch (touchscreen->rotation) {
        case 1:
        case 3:
            tsgl_pos t = x;
            x = y;
            y = touchscreen->width - t;
            break;
    }

    return (tsgl_touchscreen_point) {
        .x = x + 0.5,
        .y = y + 0.5,
        .z = z
    };
}

void tsgl_touchscreen_free(tsgl_touchscreen* touchscreen) {
    free(touchscreen->ts);
}