#include "TSGL.h"
#include "TSGL_display.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include "TSGL_spi.h"
#include "TSGL_ledc.h"
#include "TSGL_font.h"
#include "TSGL_math.h"
#include "TSGL_gfx.h"

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

static bool TSGL_FAST_FUNC _pointInFrame(tsgl_display* display, tsgl_pos x, tsgl_pos y) {
    return x >= display->viewport_minX && y >= display->viewport_minY && x < display->viewport_maxX && y < display->viewport_maxY;
}

static void TSGL_FAST_FUNC _spi_sendCommand(tsgl_display* display, const uint8_t cmd) {
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

static void TSGL_FAST_FUNC _spi_sendData(tsgl_display* display, const uint8_t* data, size_t size) {
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
        #ifdef CONFIG_IDF_TARGET_ESP32
            size_t part = tsgl_getPartSize() / 2;
        #else
            size_t part = 1024 * 16;
        #endif
        uint8_t* buffer1 = malloc(part);
        uint8_t* buffer2 = malloc(part);
        size_t offset = 0;
        bool currentBuffer = false;
        spi_transaction_t partTransaction;
        bool oldPartTransactionExists = false;
        while (true) {
            uint8_t* buffer = currentBuffer ? buffer2 : buffer1;
            size_t len = TSGL_MATH_MIN(size - offset, part);
            memcpy(buffer, data + offset, len);

            if (oldPartTransactionExists) {
                spi_transaction_t* partTransactionPtr = &partTransaction;
                ESP_ERROR_CHECK(spi_device_get_trans_result(*interfaceData->spi, &partTransactionPtr, portMAX_DELAY));
                oldPartTransactionExists = false;
            }

            partTransaction = (spi_transaction_t) {
                .length = len * 8,
                .tx_buffer = buffer,
                .user = (void*)(&pre_transfer_info)
            };

            ESP_ERROR_CHECK(spi_device_queue_trans(*interfaceData->spi, &partTransaction, portMAX_DELAY));
            oldPartTransactionExists = true;
            currentBuffer = !currentBuffer;

            offset += part;
            if (offset >= size) {
                break;
            }
        }
        if (oldPartTransactionExists) {
            spi_transaction_t* partTransactionPtr = &partTransaction;
            ESP_ERROR_CHECK(spi_device_get_trans_result(*interfaceData->spi, &partTransactionPtr, portMAX_DELAY));
        }
        free(buffer1);
        free(buffer2);
    }
}

static bool TSGL_FAST_FUNC _doCommand(tsgl_display* display, const tsgl_driver_command command) {
    tsgl_display_sendCommandWithArgs(display, command.cmd, command.data, command.datalen);
    
    if (command.delay > 0) {
        vTaskDelay(command.delay / portTICK_PERIOD_MS);
    } else if (command.delay < 0) {
        return true;
    }
    return false;
}

static void TSGL_FAST_FUNC _doCommands(tsgl_display* display, const tsgl_driver_command* list) {
    uint16_t cmd = 0;
    while (!_doCommand(display, list[cmd++]));
}

static void TSGL_FAST_FUNC _doCommandList(tsgl_display* display, tsgl_driver_list list) {
    _doCommands(display, list.list);
}

static void TSGL_FAST_FUNC _spi_pre_transfer_callback(spi_transaction_t* t) {
    spi_pretransfer_info* pretransfer_info = (spi_pretransfer_info*)t->user;
    gpio_set_level(pretransfer_info->pin, pretransfer_info->state);
}

static bool TSGL_FAST_FUNC _spi_floodCallback(void* arg, void* data, size_t size) {
    _spi_sendData((tsgl_display*)arg, data, size);
    return true;
}

static bool TSGL_FAST_FUNC _lcd_floodCallback(void* arg, void* data, size_t size) {
    tsgl_display_interfaceData_lcd* interfaceData = ((tsgl_display*)arg)->interface;
    return esp_lcd_panel_io_tx_color(*interfaceData->lcd, -1, data, size) == ESP_OK;
}

static void TSGL_FAST_FUNC _select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    if (display->driver->select != NULL) {
        _doCommandList(display,
            display->driver->select(&display->storage,
                display->offsetX + x,
                display->offsetY + y,
                (display->offsetX + x + width) - 1,
                (display->offsetY + y + height) - 1
            )
        );
    }
}

static void TSGL_FAST_FUNC _selectAll(tsgl_display* display) {
    _select(display, 0, 0, display->width, display->height);
}

esp_err_t tsgl_display_spi(tsgl_display* display, const tsgl_display_settings settings, spi_host_device_t spihost, size_t freq, gpio_num_t dc, gpio_num_t cs, gpio_num_t rst) {
    memset(display, 0, sizeof(tsgl_display));
    memcpy(&display->storage, &settings.driver->storage, sizeof(tsgl_driver_storage));
    display->storage.swapRGB = settings.swapRGB;
    display->storage.flipX = settings.flipX;
    display->storage.flipY = settings.flipY;
    display->storage.flipXY = settings.flipXY;
    display->storage.display = display;

    display->invertBacklight = settings.invertBacklight;
    if (settings.backlight_init)
        tsgl_display_attachBacklight(display, settings.backlight_pin, settings.backlight_value);

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
    display->incompleteSending = settings.driver->incompleteSending;
    tsgl_display_clrViewport(display);

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
            .trans_queue_depth = 128,
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
            .input_delay_ns = 0,
            .queue_size = 2,
            .pre_cb = _spi_pre_transfer_callback,
            .flags = SPI_DEVICE_NO_DUMMY
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
            if (settings.driver->resetHighTime >= 0) tsgl_delay(settings.driver->resetHighTime != 0 ? settings.driver->resetHighTime : 50);
            gpio_set_level(rst, true);
            if (settings.driver->resetLowTime >= 0) tsgl_delay(settings.driver->resetLowTime != 0 ? settings.driver->resetLowTime : 50);
        }

        // init display
        _doCommands(display, settings.driver->init);
        tsgl_display_setInvert(display, false);
        if (display->driver->rotate != NULL) {
            tsgl_display_rotate(display, settings.init_framebuffer_rotation);
        }
        
        switch (settings.init_state) {
            case tsgl_display_init_color:
                tsgl_display_clear(display, settings.init_color);
                break;

            case tsgl_display_init_framebuffer:
                tsgl_display_sendData(display, settings.init_framebuffer_ptr, settings.init_framebuffer_size);
                break;
            
            default:
                tsgl_display_clear(display, display->black);
                break;
        }

        // enable display
        if (settings.driver->enable != NULL) {
            _doCommandList(display, settings.driver->enable(&display->storage, true));
        }

        if (display->driver->rotate != NULL) {
            tsgl_display_rotate(display, 0);
        } else {
            _selectAll(display);
        }
    } else {
        tsgl_display_free(display);
    }

    return result;
}

void tsgl_display_rotate(tsgl_display* display, uint8_t rotation) {
    if (display->driver->rotate != NULL) {
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
        _doCommandList(display, display->driver->rotate(&display->storage, rotation));
        _selectAll(display);
        tsgl_display_clrViewport(display);
    } else {
        ESP_LOGE(TAG, "your display does not support tsgl_display_rotate");
    }
}

void tsgl_display_resetPointer(tsgl_display* display) {
    if (display->driver->pointer != NULL && display->driver->flatPointer != NULL) {
        tsgl_display_flatPointer(display, 0);
    } else {
        tsgl_display_selectAll(display);
    }
}

void tsgl_display_pointer(tsgl_display* display, tsgl_pos x, tsgl_pos y) {
    x = display->offsetX + x;
    y = display->offsetY + y;
    if (display->driver->pointer != NULL) {
        _doCommandList(display,
            display->driver->pointer(&display->storage, x, y)
        );
    } else if (display->driver->flatPointer != NULL) {
        tsgl_display_flatPointer(display, x + (y * display->width));
    } else {
        ESP_LOGE(TAG, "your display does not support tsgl_display_pointer");
    }
}

void tsgl_display_flatPointer(tsgl_display* display, size_t index) {
    if (display->driver->flatPointer != NULL) {
        _doCommandList(display,display->driver->flatPointer(&display->storage, index));
    } else {
        tsgl_display_pointer(display, index % display->width, index / display->width);
    }
}

void tsgl_display_selectAll(tsgl_display* display) {
    tsgl_display_select(display, 0, 0, display->width, display->height);
}

void tsgl_display_select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    if (display->driver->select != NULL) {
        _select(display, x, y, width, height);
    } else {
        ESP_LOGE(TAG, "your display does not support tsgl_display_select");
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
            size_t part = 2048;
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

void tsgl_display_endCommands(tsgl_display* display) {
    if (display->driver->selectAreaAfterCommand) _selectAll(display);
    if (display->driver->endCommands != NULL) _doCommandList(display, display->driver->endCommands(&display->storage));
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

void tsgl_display_incompleteSending(tsgl_display* display, bool enable, tsgl_framebuffer* checkbuffer) {
    display->incompleteSending = enable;
    display->checkbuffer = checkbuffer;
}

void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer) {
    if (framebuffer->changed) {
        if (display->incompleteSending) {
            if (framebuffer->width != display->width || framebuffer->height != display->height) {
                ESP_LOGW(TAG, "you have incompleteSending enabled, and the framebuffer size does not match the screen size. MAY LEAD TO UB");
            }
            if (framebuffer->softwareRotate) {
                ESP_LOGW(TAG, "you have incompleteSending enabled, and a software buffer rotation was used. MAY LEAD TO UB");
            }

            //printf("%i %i %i %i\n", framebuffer->changedLeft, framebuffer->changedUp, (framebuffer->changedRight - framebuffer->changedLeft) + 1, (framebuffer->changedDown - framebuffer->changedUp) + 1);
            //printf("zone: %li %li\n", framebuffer->changedFrom, (framebuffer->changedTo - framebuffer->changedFrom) + 1);
            
            //tsgl_display_select(display, framebuffer->changedLeft, framebuffer->changedUp, (framebuffer->changedRight - framebuffer->changedLeft) + 1, (framebuffer->changedDown - framebuffer->changedUp) + 1);
            //tsgl_display_sendData(display, framebuffer->buffer + framebuffer->changedFrom, (framebuffer->changedTo - framebuffer->changedFrom) + 1);

            //tsgl_display_flatPointer(display, framebuffer->changedFrom);
            //tsgl_display_sendData(display, framebuffer->buffer + framebuffer->changedFrom, (framebuffer->changedTo - framebuffer->changedFrom) + 1);

            if (framebuffer->changedLeft == 0 && framebuffer->changedUp == 0 && framebuffer->changedRight == (framebuffer->width - 1) && framebuffer->changedDown == (framebuffer->height - 1)) {
                tsgl_display_sendData(display, framebuffer->buffer, framebuffer->buffersize);
            } else {
                tsgl_pos xLine = (framebuffer->changedRight - framebuffer->changedLeft) + 1;
                tsgl_pos yLine = (framebuffer->changedDown - framebuffer->changedUp) + 1;
                tsgl_pos sendSize = xLine * framebuffer->colorsize;
                tsgl_display_select(display, framebuffer->changedLeft, framebuffer->changedUp, xLine, yLine);
                for (tsgl_pos iy = 0; iy < yLine; iy++) {
                    tsgl_display_sendData(display, framebuffer->buffer + ((framebuffer->changedLeft + (framebuffer->changedUp * display->width)) * 2) + ((size_t)(display->width * iy * framebuffer->colorsize)), sendSize);
                }
            }
        } else {
            tsgl_display_sendData(display, framebuffer->buffer, framebuffer->buffersize);
        }
        tsgl_framebuffer_resetChangedArea(framebuffer);
    }
}

void tsgl_display_setEnable(tsgl_display* display, bool state) {
    display->enable = state;
    if (display->driver->enable) {
        _doCommandList(display, display->driver->enable(&display->storage, state));
        if (state) {
            tsgl_display_endCommands(display);
        }
    }
}

void tsgl_display_setInvert(tsgl_display* display, bool state) {
    if (display->driver->invert != NULL) {
        _doCommandList(display, display->driver->invert(&display->storage, state ^ display->baseInvert));
        tsgl_display_endCommands(display);
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
        tsgl_display_endCommands(display);
    }
}

// async send

static bool asyncSendActive = false;

typedef struct {
    tsgl_display* display;
    tsgl_framebuffer* buffer;
} _asyncData;

static void _asyncSend(void* buffer) {
    _asyncData* data = (_asyncData*)buffer;
    tsgl_display_send(data->display, data->buffer);
    free(data);
    asyncSendActive = false;
    vTaskDelete(NULL);
} 

void tsgl_display_asyncSend(tsgl_display* display, tsgl_framebuffer* framebuffer, tsgl_framebuffer* framebuffer2) {
    _asyncData* data = (_asyncData*)malloc(sizeof(_asyncData));
    data->display = display;
    data->buffer = framebuffer;

    uint8_t* t = framebuffer2->buffer;
    framebuffer2->buffer = framebuffer->buffer;
    framebuffer->buffer = t;

    while (asyncSendActive) vTaskDelay(1);
    asyncSendActive = true;
    xTaskCreate(_asyncSend, NULL, 4096, (void*)data, 24, NULL);
}

void tsgl_display_asyncCopySend(tsgl_display* display, tsgl_framebuffer* framebuffer, tsgl_framebuffer* framebuffer2) {
    if (!framebuffer->changed) return;
    while (asyncSendActive) vTaskDelay(1);
    asyncSendActive = true;

    memcpy(framebuffer2->buffer + framebuffer->changedFrom, framebuffer->buffer + framebuffer->changedFrom, framebuffer->changedTo - framebuffer->changedFrom);

    framebuffer2->changed = framebuffer->changed;
    framebuffer2->changedFrom = framebuffer->changedFrom;
    framebuffer2->changedTo = framebuffer->changedTo;
    framebuffer2->changedLeft = framebuffer->changedLeft;
    framebuffer2->changedRight = framebuffer->changedRight;
    framebuffer2->changedUp = framebuffer->changedUp;
    framebuffer2->changedDown = framebuffer->changedDown;
    framebuffer2->rotation = framebuffer->rotation;
    framebuffer2->realRotation = framebuffer->realRotation;
    framebuffer2->hardwareRotate = framebuffer->hardwareRotate;
    framebuffer2->softwareRotate = framebuffer->softwareRotate;
    framebuffer2->width = framebuffer->width;
    framebuffer2->height = framebuffer->height;

    _asyncData* data = (_asyncData*)malloc(sizeof(_asyncData));
    data->display = display;
    data->buffer = framebuffer2;

    xTaskCreate(_asyncSend, NULL, 4096, (void*)data, 24, NULL);
}

void tsgl_display_dumpViewport(tsgl_display* display, tsgl_viewport_dump* dump) {
    dump->viewport = display->viewport;
    dump->viewport_minX = display->viewport_minX;
    dump->viewport_minY = display->viewport_minY;
    dump->viewport_maxX = display->viewport_maxX;
    dump->viewport_maxY = display->viewport_maxY;
}

void tsgl_display_flushViewport(tsgl_display* display, tsgl_viewport_dump* dump) {
    display->viewport = dump->viewport;
    display->viewport_minX = dump->viewport_minX;
    display->viewport_minY = dump->viewport_minY;
    display->viewport_maxX = dump->viewport_maxX;
    display->viewport_maxY = dump->viewport_maxY;
}

void tsgl_display_clrViewport(tsgl_display* display) {
    tsgl_display_setViewport(display, 0, 0, display->width, display->height);
    display->viewport = false;
}

void tsgl_display_setViewport(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    display->viewport = x != 0 || y != 0 || width != display->width || height != display->height;
    display->viewport_minX = x;
    display->viewport_minY = y;
    display->viewport_maxX = x + width;
    display->viewport_maxY = y + height;
}

void tsgl_display_setViewportRange(tsgl_display* display, tsgl_pos minX, tsgl_pos minY, tsgl_pos maxX, tsgl_pos maxY) {
    display->viewport = minX != 0 || minX != 0 || maxX != display->width || maxY != display->height;
    display->viewport_minX = minX;
    display->viewport_minY = minY;
    display->viewport_maxX = maxX;
    display->viewport_maxY = maxY;
}

// graphic

void TSGL_FAST_FUNC tsgl_display_push(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_sprite* sprite) {
    tsgl_gfx_push(display, (TSGL_SET_REFERENCE())tsgl_display_setWithoutCheck, x, y, sprite, display->viewport_minX, display->viewport_minY, display->viewport_maxX, display->viewport_maxY);
}

void TSGL_FAST_FUNC tsgl_display_setWithoutCheck(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
    if (display->driver->pointer != NULL && display->driver->flatPointer != NULL) {
        tsgl_display_pointer(display, x, y);
    } else {
        tsgl_display_select(display, x, y, 1, 1);
    }
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

void TSGL_FAST_FUNC tsgl_display_set(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
    if (!_pointInFrame(display, x, y)) return;
    tsgl_display_setWithoutCheck(display, x, y, color);
}

void TSGL_FAST_FUNC tsgl_display_line(tsgl_display* display, tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke) {
    tsgl_gfx_line(display, (TSGL_SET_REFERENCE())tsgl_display_setWithoutCheck, (TSGL_FILL_REFERENCE())tsgl_display_fillWithoutCheck, x1, y1, x2, y2, color, stroke, display->viewport_minX, display->viewport_minY, display->viewport_maxX, display->viewport_maxY);
}

void TSGL_FAST_FUNC tsgl_display_fill(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
    if (x < display->viewport_minX) {
        width = width - (display->viewport_minX - x);
        x = display->viewport_minX;
    }
    if (y < display->viewport_minY) {
        height = height - (display->viewport_minY - y);
        y = display->viewport_minY;
    }
    if (width + x > display->viewport_maxX) width = display->viewport_maxX - x;
    if (height + y > display->viewport_maxY) height = display->viewport_maxY - y;
    if (width <= 0 || height <= 0) return;
    tsgl_display_fillWithoutCheck(display, x, y, width, height, color);
}

void TSGL_FAST_FUNC tsgl_display_fillWithoutCheck(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
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

void TSGL_FAST_FUNC tsgl_display_rect(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke) {
    tsgl_gfx_rect(display, (TSGL_FILL_REFERENCE())tsgl_display_fill, x, y, width, height, color, stroke);
}

tsgl_print_textArea TSGL_FAST_FUNC tsgl_display_text(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text) {
    return tsgl_gfx_text(display, (TSGL_SET_REFERENCE())tsgl_display_setWithoutCheck, (TSGL_FILL_REFERENCE())tsgl_display_fillWithoutCheck, x, y, sets, text, display->viewport_minX, display->viewport_minY, display->viewport_maxX, display->viewport_maxY);
}

void TSGL_FAST_FUNC tsgl_display_clear(tsgl_display* display, tsgl_rawcolor color) {
    tsgl_display_fillWithoutCheck(display, 0, 0, display->width, display->height, color);
}