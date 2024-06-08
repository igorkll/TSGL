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

static void _select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    tsgl_pos x2 = 0;
    tsgl_pos y2 = 0;
    switch (display->rotation) {
        case 0:
            x2 = (x + width) - 1;
            y2 = (y + height) - 1;
            break;
        case 1:
            tsgl_pos ox = x;
            x = display->defaultWidth - y - height;
            x2 = display->defaultWidth - y - 1;
            y = ox;
            y2 = (ox + width) - 1;
            break;
        case 2:
            x2 = display->defaultWidth - x - 1;
            y2 = display->defaultHeight - y - 1;
            x = (x2 - width) + 1;
            y = (y2 - height) + 1;
            break;
        case 3:
            tsgl_pos oy = y;
            y = display->defaultHeight - x - width;
            x = oy;
            x2 = (x + height) - 1;
            y2 = (y + width) - 1;
            break;
    }
    _doCommandList(display, display->driver->select(x, y, x2, y2));
}

static void _selectLast(tsgl_display* display) {
    _select(display, display->lastSelectX, display->lastSelectY, display->lastSelectWidth, display->lastSelectHeight);
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
    display->defaultWidth = width;
    display->defaultHeight = height;
    display->rotation = 0;
    display->interfaceType = tsgl_display_interface_spi;
    display->interface = malloc(sizeof(spi_device_handle_t));
    display->dc = dc;
    display->driver = driver;
    display->colormode = driver->colormode;
    display->colorsize = tsgl_colormodeSizes[driver->colormode];
    display->black = tsgl_color_raw(TSGL_BLACK, driver->colormode);

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
        tsgl_display_selectAll(display);
    } else {
        free(display->interface);
    }
    return result;
}

void tsgl_display_rotate(tsgl_display* display, uint8_t rotation) {
    display->rotation = rotation % 4;
    switch (display->rotation) {
        case 0:
        case 2:
            display->width = display->defaultWidth;
            display->height = display->defaultHeight;
            break;
        case 1:
        case 3:
            display->width = display->defaultHeight;
            display->height = display->defaultWidth;
            break;
    }
    if (display->driver->rotate != NULL) {
        _doCommandList(display, display->driver->rotate(rotation));
    }
    _selectLast(display);
}

void tsgl_display_selectAll(tsgl_display* display) {
    tsgl_display_select(display, 0, 0, display->width, display->height);
}

void tsgl_display_select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    display->lastSelectX = x;
    display->lastSelectY = y;
    display->lastSelectWidth = width;
    display->lastSelectHeight = height;
    _select(display, x, y, width, height);
}

void tsgl_display_enable(tsgl_display* display) {
    _doCommands(display, display->driver->enable);
    _selectLast(display);
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

void tsgl_display_sendCommandWithArg(tsgl_display* display, const uint8_t command, const uint8_t arg) {
    tsgl_display_sendCommand(display, command);
    tsgl_display_sendData(display, &arg, 1);
}

void tsgl_display_sendFlood(tsgl_display* display, const uint8_t* data, size_t size, size_t flood) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            tsgl_spi_sendFlood(display, data, size, flood);
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


// graphic

void tsgl_display_set(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
    _select(display, x, y, 1, 1);
    tsgl_display_sendData(display, (const uint8_t*)color.arr, display->colorsize);
    _selectLast(display);
}

void tsgl_display_fill(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
    if (width <= 0 || height <= 0) return;
    _select(display, x, y, width, height);
    tsgl_display_sendFlood(display, (const uint8_t*)color.arr, display->colorsize, width * height);
    _selectLast(display);
}

void tsgl_display_rect(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
    tsgl_display_fill(display, x, y, width, 1, color);
    tsgl_display_fill(display, x, y + (height - 1), width, 1, color);
    tsgl_display_fill(display, x, y + 1, 1, height - 2, color);
    tsgl_display_fill(display, x + (width - 1), y + 1, 1, height - 2, color);
}

void tsgl_display_clear(tsgl_display* display, tsgl_rawcolor color) {
    tsgl_display_fill(display, 0, 0, display->width, display->height, color);
}