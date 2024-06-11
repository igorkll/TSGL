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



esp_err_t tsgl_display_spi(tsgl_display* display, const tsgl_driver* driver, const tsgl_driver_settings driver_settings, spi_host_device_t spihost, size_t freq, int8_t dc, int8_t cs, int8_t rst) {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = freq,
        .mode = 0,
        .spics_io_num = cs,
        .queue_size = 7,
        .pre_cb = tsgl_spi_pre_transfer_callback
    };

    memcpy(&display->driver_settings, &driver_settings, sizeof(driver_settings));
    display->width = driver_settings.width;
    display->height = driver_settings.height;
    display->defaultWidth = driver_settings.width;
    display->defaultHeight = driver_settings.height;
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
        tsgl_display_setInvert(display, false);
        tsgl_display_rotate(display, 0);
        tsgl_display_clear(display, display->black);

        // enable display
        _doCommands(display, driver->enable);
        tsgl_display_rotate(display, 0);
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
        //printf("r:%i\n", rotation);
        _doCommandList(display, display->driver->rotate(rotation));
    }
    tsgl_display_selectAll(display);
}

void tsgl_display_selectAll(tsgl_display* display) {
    tsgl_display_select(display, 0, 0, display->width, display->height);
}

void tsgl_display_select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    tsgl_pos offsetX = display->driver_settings.offsetX;
    tsgl_pos offsetY = display->driver_settings.offsetY;
    _doCommandList(display, display->driver->select(offsetX + x, offsetY + y, (offsetX + x + width) - 1, (offsetY + y + height) - 1));
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

void tsgl_display_setEnable(tsgl_display* display, bool state) {
    if (state) {
        _doCommands(display, display->driver->enable);
        tsgl_display_selectAll(display);
    } else {
        _doCommands(display, display->driver->disable);
    }
    display->enable = state;
}

void tsgl_display_setInvert(tsgl_display* display, bool state) {
    if (display->driver->invert != NULL) {
        _doCommandList(display, display->driver->invert(state ^ display->driver_settings.invert));
        tsgl_display_selectAll(display);
    }
    display->invert = state;
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

void tsgl_display_push(tsgl_display* display, tsgl_pos x, tsgl_pos y, uint8_t rotation, tsgl_framebuffer* sprite) {
    if (display->driver->rotate != NULL)
        _doCommandList(display, display->driver->rotate(rotation));

    tsgl_pos spriteWidth = sprite->defaultWidth;
    tsgl_pos spriteHeight = sprite->defaultHeight;
    switch (rotation) {
        case 1:
        case 3:
            spriteWidth = sprite->defaultHeight;
            spriteHeight = sprite->defaultWidth;
            break;
    }
    tsgl_display_select(display, x, y, spriteWidth, spriteHeight);
    tsgl_display_sendData(display, sprite->buffer, sprite->buffersize);

    if (display->driver->rotate != NULL)
        _doCommandList(display, display->driver->rotate(display->rotation));
}

void tsgl_display_set(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
    tsgl_display_select(display, x, y, 1, 1);
    switch (display->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            tsgl_display_sendData(display, color.arr, 2);
            break;
        
        default:
            tsgl_display_sendData(display, (const uint8_t*)color.arr, display->colorsize);
            break;
    }
}

void tsgl_display_fill(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
    if (width <= 0 || height <= 0) return;
    tsgl_display_select(display, x, y, width, height);
    switch (display->colormode) {
        case tsgl_rgb444:
        case tsgl_bgr444:
            tsgl_display_sendFlood(display, color.arr, 3, (width * height) / 2);
            break;
        
        default:
            tsgl_display_sendFlood(display, (const uint8_t*)color.arr, display->colorsize, width * height);
            break;
    }
}

void tsgl_display_rect(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos strokelen) {
    TSGL_GFX_RECT(display, tsgl_display_fill, x, y, width, height, color, strokelen);
}

void tsgl_display_clear(tsgl_display* display, tsgl_rawcolor color) {
    tsgl_display_fill(display, 0, 0, display->width, display->height, color);
}