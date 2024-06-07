#include "TSGL.h"
#include "TSGL_display.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include "TSGL_spi.h"

#include <esp_system.h>
#include <esp_err.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


static bool _doCommand(tsgl_display* display, const tsgl_driver_command command) {
    tsgl_display_sendCommand(display, command.cmd);
    if (command.datalen > 0) {
        tsgl_display_sendData(display, command.data, command.datalen);
    }
    if (command.delay > 0) {
        vTaskDelay(command.delay / portTICK_PERIOD_MS);
    } else if (command.delay < 0) {
        return true;
    }
    return false;
}

static void _doCommands(tsgl_display* display, const tsgl_driver_command* list) {
    uint16_t cmd = 0;
    while (!_doCommand(display, list[cmd++]));
}

static void _doCommandList(tsgl_display* display, tsgl_driver_list list) {
    _doCommands(display, list.list);
}

esp_err_t tsgl_display_spi(tsgl_display* display, const tsgl_driver* driver, tsgl_pos width, tsgl_pos height, spi_host_device_t spihost, size_t freq, int8_t dc, int8_t cs, int8_t rst) {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = freq,
        .mode = 0,
        .spics_io_num = cs,
        .queue_size = 7,
        .pre_cb = tsgl_spi_pre_transfer_callback
    };

    display->width = width;
    display->height = height;
    display->interfaceType = tsgl_display_interface_spi;
    display->interface = malloc(sizeof(spi_device_handle_t));
    display->dc = dc;
    display->driver = driver;

    esp_err_t result = spi_bus_add_device(spihost, &devcfg, (spi_device_handle_t*)display->interface);
    if (result == ESP_OK) {
        // configuration of non-SPI pins
        gpio_config_t io_conf = {};
        if (dc >= 0) io_conf.pin_bit_mask |= 1ULL << dc;
        if (rst >= 0) io_conf.pin_bit_mask |= 1ULL << rst;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_up_en = true;
        gpio_config(&io_conf);

        // reset display
        if (rst >= 0) {
            gpio_set_level(rst, false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_set_level(rst, true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        // init display
        _doCommands(display, driver->init);
        tsgl_display_select(display, 0, 0, width, height);
    } else {
        free(display->interface);
    }
    return result;
}

void tsgl_display_select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    _doCommandList(display, display->driver->select(x, y, (x + width) - 1, (y + height) - 1));
}

void tsgl_display_enable(tsgl_display* display) {
    _doCommands(display, display->driver->enable);
}

void tsgl_display_sendCommand(tsgl_display* display, const uint8_t command) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            tsgl_spi_sendCommand(display, command);
            break;
    }
}

void tsgl_display_sendData(tsgl_display* display, const uint8_t* data, size_t size) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            tsgl_spi_sendData(display, data, size);
            break;
    }
}

void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer) {
    tsgl_display_sendData(display, framebuffer->buffer, framebuffer->buffersize);
}

void tsgl_display_disable(tsgl_display* display) {
    _doCommands(display, display->driver->disable);
}

void tsgl_display_free(tsgl_display* display) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            spi_bus_remove_device(*((spi_device_handle_t*)display->interface));
            break;
    }
    free(display->interface);
}