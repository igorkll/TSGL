#include "TSGL.h"
#include "TSGL_display.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include "TSGL_spi.h"
#include "TSGL_gfx.h"
#include "TSGL_ledc.h"
#include "TSGL_font.h"

#include <esp_system.h>
#include <esp_err.h>
#include <esp_log.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "TSGL_display";

typedef struct {
    gpio_num_t pin;
    bool state;
} spi_pretransfer_info;

static void _spi_sendCommand(tsgl_display* display, const uint8_t cmd) {
    tsgl_display_interfaceData_spi* interfaceData = display->interface;

    spi_pretransfer_info pre_transfer_info = {
        .pin = interfaceData->dc,
        .state = false
    };

    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
        .user = (void*)(&pre_transfer_info)
    };

    ESP_ERROR_CHECK(spi_device_transmit(*interfaceData->spi, &t));
}

static void _spi_sendData(tsgl_display* display, const uint8_t* data, size_t size) {
    tsgl_display_interfaceData_spi* interfaceData = display->interface;

    spi_pretransfer_info pre_transfer_info = {
        .pin = interfaceData->dc,
        .state = true
    };

    spi_transaction_t transaction = {
        .length = size * 8,
        .tx_buffer = data,
        .user = (void*)(&pre_transfer_info)
    };

    if (spi_device_transmit(*interfaceData->spi, &transaction) != ESP_OK) {
        size_t part = tsgl_getPartSize();
        size_t offset = 0;
        uint8_t* buffer = malloc(part);
        while (true) {
            size_t len = TSGL_MATH_MIN(size - offset, part);
            spi_transaction_t partTransaction = {
                .length = len * 8,
                .tx_buffer = buffer,
                .user = (void*)(&pre_transfer_info)
            };
            memcpy(buffer, data + offset, len);
            ESP_ERROR_CHECK(spi_device_transmit(*interfaceData->spi, &partTransaction));
            offset += part;
            if (offset >= size) {
                break;
            }
        }
        free(buffer);
    }
}

static bool _doCommand(tsgl_display* display, const tsgl_driver_command command) {
    tsgl_display_sendCommandWithArgs(display, command.cmd, command.data, command.datalen);
    
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

static void _spi_pre_transfer_callback(spi_transaction_t* t) {
    spi_pretransfer_info* pretransfer_info = (spi_pretransfer_info*)t->user;
    gpio_set_level(pretransfer_info->pin, pretransfer_info->state);
}

static bool _spi_floodCallback(void* arg, void* data, size_t size) {
    _spi_sendData((tsgl_display*)arg, data, size);
    return true;
}

static bool _lcd_floodCallback(void* arg, void* data, size_t size) {
    tsgl_display_interfaceData_lcd* interfaceData = ((tsgl_display*)arg)->interface;
    return esp_lcd_panel_io_tx_color(*interfaceData->lcd, -1, data, size) == ESP_OK;
}



static uint8_t initType = 0;
static tsgl_rawcolor initColor;
static const uint8_t* initFramebuffer;
static size_t initFramebufferSize;
static uint8_t initRotation;
static gpio_num_t initBlPin = -1;
static gpio_num_t initBlDefaultValue = -1;

void tsgl_display_pushInitColor(tsgl_rawcolor color) {
    initColor = color;
    initRotation = 0;
    initType = 1;
}

void tsgl_display_pushInitFramebuffer(tsgl_framebuffer* framebuffer, uint8_t rotation) {
    initFramebuffer = framebuffer->buffer;
    initFramebufferSize = framebuffer->buffersize;
    initRotation = rotation;
    initType = 2;
}

void tsgl_display_pushInitRawFramebuffer(const uint8_t* framebuffer, size_t size, uint8_t rotation) {
    initFramebuffer = framebuffer;
    initFramebufferSize = size;
    initRotation = rotation;
    initType = 2;
}

void tsgl_display_pushBacklight(gpio_num_t pin, uint8_t defaultValue) {
    initBlPin = pin;
    initBlDefaultValue = defaultValue;
}


esp_err_t tsgl_display_spi(tsgl_display* display, const tsgl_settings settings, spi_host_device_t spihost, size_t freq, gpio_num_t dc, gpio_num_t cs, gpio_num_t rst) {
    memset(display, 0, sizeof(tsgl_display));
    memcpy(&display->storage, &settings.driver->storage, sizeof(tsgl_driver_storage));

    display->invertBacklight = settings.invertBacklight;
    if (initBlPin >= 0)
        tsgl_display_attachBacklight(display, initBlPin, initBlDefaultValue);
    display->storage.swapRGB = settings.swapRGB;
    display->storage.flipX = settings.flipX;
    display->storage.flipY = settings.flipY;
    display->storage.flipXY = settings.flipXY;
    display->baseInvert = settings.invert;
    display->offsetX = settings.offsetX;
    display->offsetY = settings.offsetY;
    display->width = settings.width;
    display->height = settings.height;
    display->defaultWidth = settings.width;
    display->defaultHeight = settings.height;
    display->rotation = 0;
    display->driver = settings.driver;
    display->colormode = settings.driver->colormode;
    display->colorsize = tsgl_colormodeSizes[display->colormode];
    display->black = tsgl_color_raw(TSGL_BLACK, display->colormode);

    esp_err_t result;
    if (false) {
        tsgl_display_interfaceData_lcd* interfaceData = malloc(sizeof(tsgl_display_interfaceData_lcd));
        display->interfaceType = tsgl_display_interface_lcd;
        display->interface = interfaceData;

        esp_lcd_panel_io_spi_config_t io_config = {
            .dc_gpio_num = dc,
            .cs_gpio_num = cs,
            .pclk_hz = freq,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .spi_mode = 0,
            .trans_queue_depth = 16,
        };

        interfaceData->lcd = malloc(sizeof(esp_lcd_panel_io_handle_t));
        result = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)spihost, &io_config, interfaceData->lcd);
    } else {
        tsgl_display_interfaceData_spi* interfaceData = malloc(sizeof(tsgl_display_interfaceData_spi));
        display->interfaceType = tsgl_display_interface_spi;
        display->interface = interfaceData;
        interfaceData->dc = dc;

        spi_device_interface_config_t devcfg = {
            .clock_speed_hz = freq,
            .mode = 0,
            .spics_io_num = cs,
            .queue_size = 1,
            .pre_cb = _spi_pre_transfer_callback
        };

        interfaceData->spi = malloc(sizeof(spi_device_handle_t));
        result = spi_bus_add_device(spihost, &devcfg, interfaceData->spi);
    }

    if (result == ESP_OK) {
        // configuration of non-SPI pins
        gpio_config_t io_conf = {};
        if (dc >= 0) io_conf.pin_bit_mask |= 1ULL << dc;
        if (rst >= 0) io_conf.pin_bit_mask |= 1ULL << rst;
        io_conf.mode = GPIO_MODE_OUTPUT;
        gpio_config(&io_conf);

        // reset display
        if (rst >= 0) {
            gpio_set_level(rst, false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_set_level(rst, true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        // init display
        _doCommands(display, settings.driver->init);
        tsgl_display_setInvert(display, false);
        tsgl_display_rotate(display, initRotation);
        switch (initType) {
            case 1:
                tsgl_display_clear(display, initColor);
                break;

            case 2:
                tsgl_display_sendData(display, initFramebuffer, initFramebufferSize);
                break;
            
            default:
                tsgl_display_clear(display, display->black);
                break;
        }

        // enable display
        if (settings.driver->enable != NULL) {
            _doCommandList(display, settings.driver->enable(&display->storage, true));
        }
        tsgl_display_rotate(display, 0);
    } else {
        tsgl_display_free(display);
    }
    initType = 0;
    initBlPin = -1;
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
        _doCommandList(display, display->driver->rotate(&display->storage, rotation));
    }
    tsgl_display_selectAll(display);
}

void tsgl_display_selectAll(tsgl_display* display) {
    tsgl_display_select(display, 0, 0, display->width, display->height);
}

void tsgl_display_selectIfNeed(tsgl_display* display) {
    if (display->driver->selectAreaAfterCommand) {
        tsgl_display_selectAll(display);
    }
}

void tsgl_display_select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    if (display->driver->select != NULL) {
        _doCommandList(display,
            display->driver->select(&display->storage,
            display->offsetX + x,
            display->offsetY + y, (display->offsetX + x + width) - 1,
            (display->offsetY + y + height) - 1)
        );
    }
}

void tsgl_display_sendCommand(tsgl_display* display, const uint8_t command) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            _spi_sendCommand(display, command);
            break;

        case tsgl_display_interface_lcd:
            tsgl_display_interfaceData_lcd* interfaceData = display->interface;
            esp_lcd_panel_io_tx_param(*interfaceData->lcd, command, NULL, 0);
            break;
    }
}

void tsgl_display_sendData(tsgl_display* display, const uint8_t* data, size_t size) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            _spi_sendData(display, data, size);
            break;

        case tsgl_display_interface_lcd:
            tsgl_display_interfaceData_lcd* interfaceData = display->interface;
            size_t part = 512;
            size_t offset = 0;
            while (true) {
                if (esp_lcd_panel_io_tx_color(*interfaceData->lcd, -1, data + offset, TSGL_MATH_MIN(size - offset, part)) != ESP_OK) {
                    part /= 2;
                    if (part == 0) break;
                    continue;
                }
                offset += part;
                if (offset >= size) {
                    break;
                }
            }
            break;
    }
}

void tsgl_display_sendCommandWithArg(tsgl_display* display, const uint8_t command, const uint8_t arg) {
    tsgl_display_sendCommandWithArgs(display, command, &arg, 1);
}

void tsgl_display_sendCommandWithArgs(tsgl_display* display, const uint8_t command, const uint8_t* args, size_t argsCount) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            tsgl_display_sendCommand(display, command);
            if (argsCount > 0) {
                tsgl_display_sendData(display, args, argsCount);
            }
            break;

        case tsgl_display_interface_lcd:
            tsgl_display_interfaceData_lcd* interfaceData = display->interface;
            esp_lcd_panel_io_tx_param(*interfaceData->lcd, command, args, argsCount);
            break;
    }
}

void tsgl_display_sendFlood(tsgl_display* display, const uint8_t* data, size_t size, size_t flood) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi : {
            tsgl_sendFlood(0, display, _spi_floodCallback, data, size, flood);
            break;
        }

        case tsgl_display_interface_lcd : {
            tsgl_sendFlood(0, display, _lcd_floodCallback, data, size, flood);
            break;
        }
    }
}

void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer) {
    tsgl_display_sendData(display, framebuffer->buffer, framebuffer->buffersize);
}

void tsgl_display_setEnable(tsgl_display* display, bool state) {
    display->enable = state;
    if (display->driver->enable) {
        _doCommandList(display, display->driver->enable(&display->storage, state));
        if (state) {
            tsgl_display_selectIfNeed(display);
        }
    }
}

void tsgl_display_setInvert(tsgl_display* display, bool state) {
    if (display->driver->invert != NULL) {
        _doCommandList(display, display->driver->invert(&display->storage, state ^ display->baseInvert));
        tsgl_display_selectIfNeed(display);
    }
    display->invert = state;
}

void tsgl_display_free(tsgl_display* display) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi : {
            tsgl_display_interfaceData_spi* interfaceData = display->interface;
            spi_bus_remove_device(*interfaceData->spi);
            free(interfaceData->spi);
            break;
        }

        case tsgl_display_interface_lcd : {
            tsgl_display_interfaceData_lcd* interfaceData = display->interface;
            esp_lcd_panel_io_del(*interfaceData->lcd);
            free(interfaceData->lcd);
            break;
        }
    }

    if (display->backlight != NULL) {
        tsgl_ledc_free(display->backlight);
    }

    free(display->interface);
}

esp_err_t tsgl_display_attachBacklight(tsgl_display* display, gpio_num_t pin, uint8_t defaultValue) {
    display->backlight = malloc(sizeof(tsgl_ledc));
    if (tsgl_ledc_new(display->backlight, pin, display->invertBacklight, defaultValue) != ESP_OK) {
        free(display->backlight);
        display->backlight = NULL;
        ESP_LOGE(TAG, "failed to allocate ledc on GPIO: %i", pin);
        return ESP_FAIL;
    }
    return ESP_OK;
}

void tsgl_display_setBacklight(tsgl_display* display, uint8_t value) {
    if (display->backlight != NULL) {
        tsgl_ledc_set(display->backlight, value);
    } else if (display->driver->backlight != NULL) {
        if (display->invertBacklight) {
            _doCommandList(display, display->driver->backlight(&display->storage, 255 - value));
        } else {
            _doCommandList(display, display->driver->backlight(&display->storage, value));
        }
        tsgl_display_selectIfNeed(display);
    }
}

// async send

static bool asyncSendActive = false;

typedef struct {
    tsgl_display* display;
    uint8_t* buffer;
    size_t buffersize;
} _asyncData;

static void _asyncSend(void* buffer) {
    _asyncData* data = (_asyncData*)buffer;
    tsgl_display_sendData(data->display, data->buffer, data->buffersize);
    free(data);
    asyncSendActive = false;
    vTaskDelete(NULL);
} 

void tsgl_display_asyncSend(tsgl_display* display, tsgl_framebuffer* framebuffer, tsgl_framebuffer* framebuffer2) {
    _asyncData* data = (_asyncData*)malloc(sizeof(_asyncData));
    data->display = display;
    data->buffer = framebuffer->buffer;
    data->buffersize = framebuffer->buffersize;

    uint8_t* t = framebuffer2->buffer;
    framebuffer2->buffer = framebuffer->buffer;
    framebuffer->buffer = t;

    while (asyncSendActive) vTaskDelay(1);
    asyncSendActive = true;
    xTaskCreate(_asyncSend, NULL, 4096, (void*)data, 1, NULL);
}

void tsgl_display_asyncCopySend(tsgl_display* display, tsgl_framebuffer* framebuffer, tsgl_framebuffer* framebuffer2) {
    memcpy(framebuffer2->buffer, framebuffer->buffer, framebuffer->buffersize);

    _asyncData* data = (_asyncData*)malloc(sizeof(_asyncData));
    data->display = display;
    data->buffer = framebuffer2->buffer;
    data->buffersize = framebuffer2->buffersize;

    while (asyncSendActive) vTaskDelay(1);
    asyncSendActive = true;
    xTaskCreate(_asyncSend, NULL, 4096, (void*)data, 1, NULL);
}

// graphic

void tsgl_display_push(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_sprite* sprite) {
    tsgl_gfx_push(display, (TSGL_SET_REFERENCE())tsgl_display_setWithoutCheck, x, y, sprite, display->width, display->height);
}

void tsgl_display_setWithoutCheck(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
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

void tsgl_display_set(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
    tsgl_display_setWithoutCheck(display, x, y, color);
}

void tsgl_display_line(tsgl_display* display, tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke) {
    tsgl_gfx_line(display, (TSGL_SET_REFERENCE())tsgl_display_setWithoutCheck, (TSGL_FILL_REFERENCE())tsgl_display_fillWithoutCheck, x1, y1, x2, y2, color, stroke, display->width, display->height);
}

void tsgl_display_fill(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
    if (x < 0) {
        width = width + x;
        x = 0;
    }
    if (y < 0) {
        height = height + y;
        y = 0;
    }
    if (width + x > display->width) width = display->width - x;
    if (height + y > display->height) height = display->height - y;
    if (width <= 0 || height <= 0) return;
    tsgl_display_fillWithoutCheck(display, x, y, width, height, color);
}

void tsgl_display_fillWithoutCheck(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
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

void tsgl_display_rect(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke) {
    tsgl_gfx_rect(display, (TSGL_FILL_REFERENCE())tsgl_display_fill, x, y, width, height, color, stroke);
}

tsgl_print_textArea tsgl_display_text(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text) {
    return tsgl_gfx_text(display, (TSGL_SET_REFERENCE())tsgl_display_setWithoutCheck, (TSGL_FILL_REFERENCE())tsgl_display_fillWithoutCheck, x, y, display->width, display->height, sets, text);
}

void tsgl_display_clear(tsgl_display* display, tsgl_rawcolor color) {
    tsgl_display_fillWithoutCheck(display, 0, 0, display->width, display->height, color);
}