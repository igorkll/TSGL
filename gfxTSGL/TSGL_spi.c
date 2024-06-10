#include "TSGL.h"
#include "TSGL_spi.h"
#include "TSGL_display.h"

#include <esp_system.h>
#include <esp_err.h>
#include <esp_log.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>

static const char* TAG = "tsgl_spi";

typedef struct {
    int8_t pin;
    bool state;
} Pre_transfer_info;

static size_t _getPartSize() {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    return info.largest_free_block / 2;
}

esp_err_t tsgl_spi_init(size_t maxlen, spi_host_device_t host) {
    int8_t miso;
    int8_t mosi;
    int8_t clk;
    switch (host) {
        #ifdef TSGL_HOST1
            case TSGL_HOST1:
                miso = TSGL_HOST1_MISO;
                mosi = TSGL_HOST1_MOSI;
                clk  = TSGL_HOST1_CLK;
                break;
        #endif

        #ifdef TSGL_HOST2
            case TSGL_HOST2:
                miso = TSGL_HOST2_MISO;
                mosi = TSGL_HOST2_MOSI;
                clk  = TSGL_HOST2_CLK;
                break;
        #endif
        
        default:
            ESP_LOGE(TAG, "%i spihost is unknown, tsgl_spi_init cannot be used", host);
            return ESP_ERR_INVALID_ARG;
    }
    return tsgl_spi_initManual(maxlen, host, mosi, miso, clk);
}

esp_err_t tsgl_spi_initManual(size_t maxlen, spi_host_device_t host, int8_t mosi, int8_t miso, int8_t clk) {
    spi_bus_config_t buscfg={
        .miso_io_num=miso,
        .mosi_io_num=mosi,
        .sclk_io_num=clk,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz = maxlen
    };
    return spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
}

void tsgl_spi_sendCommand(tsgl_display* display, const uint8_t cmd) {
    Pre_transfer_info pre_transfer_info = {
        .pin = display->dc,
        .state = false
    };
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
        .user = (void*)(&pre_transfer_info)
    };
    ESP_ERROR_CHECK(spi_device_transmit(*((spi_device_handle_t*)display->interface), &t));
}

#define umin(a,b) (((a) < (b)) ? (a) : (b))
void tsgl_spi_sendData(tsgl_display* display, const uint8_t* data, size_t size) {
    if (size <= 0) return;
    Pre_transfer_info pre_transfer_info = {
        .pin = display->dc,
        .state = true
    };

    spi_transaction_t t = {
        .length = size * 8,
        .tx_buffer = data,
        .user = (void*)(&pre_transfer_info)
    };

    if (spi_device_transmit(*((spi_device_handle_t*)display->interface), &t) != ESP_OK) {
        size_t part = _getPartSize();
        size_t offset = 0;
        while (true) {
            spi_transaction_t t = {
                .length = umin(size - offset, part) * 8,
                .tx_buffer = data + offset,
                .user = (void*)(&pre_transfer_info)
            };

            ESP_ERROR_CHECK(spi_device_transmit(*((spi_device_handle_t*)display->interface), &t));
            offset += part;
            if (offset >= size) {
                break;
            }
        }
    }
}

void tsgl_spi_sendFlood(tsgl_display* display, const uint8_t* data, size_t size, size_t flood) {
    if (size <= 0 || flood <= 0) return;
    size_t part = (_getPartSize() / size) * size;
    size_t bytesCount = flood * size;
    size_t offset = 0;
    void* floodPart = malloc(part);
    if (floodPart == NULL) return;
    for (size_t i = 0; i < part; i += size) {
        memcpy(floodPart + i, data, size);
    }
    while (true) {
        tsgl_spi_sendData(display, floodPart, umin(bytesCount - offset, part));
        offset += part;
        if (offset >= bytesCount) {
            break;
        }
    }
    free(floodPart);
}

void tsgl_spi_pre_transfer_callback(spi_transaction_t* t) {
    Pre_transfer_info* pre_transfer_info = (Pre_transfer_info*)t->user;
    gpio_set_level(pre_transfer_info->pin, pre_transfer_info->state);
}