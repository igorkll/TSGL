#include "TSGL.h"
#include "TSGL_spi.h"
#include "TSGL_display.h"

#include <esp_system.h>
#include <esp_err.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

typedef static struct {
    int8_t pin;
    bool state;
} Dc_info;

void tsgl_spi_pre_transfer_callback(spi_transaction_t* t) {
    Dc_info* dc_info = (Dc_info*)t->user;
    gpio_set_level(dc_info->pin, dc_info->state);
}

void tsgl_spi_sendCommand(tsgl_display* display, const uint8_t cmd) {
    Dc_info dc_info = {
        pin = display->dc,
        state = false
    };
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
        .user = (void*)dc_info
    };
    spi_device_polling_transmit((spi_device_handle_t)*display->interface, &t);
}

void tsgl_spi_sendData(tsgl_display* display, const uint8_t* data, size_t size) {
    if (size == 0) return;
    Dc_info dc_info = {
        pin = display->dc,
        state = true
    };
    spi_transaction_t t = {
        .length = size * 8,
        .tx_buffer = data,
        .user = (void*)dc_info
    };
    spi_device_polling_transmit(spi, &t);
}