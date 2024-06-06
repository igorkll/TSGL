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

static const char* TAG = "tsgl_spi";

typedef struct {
    int8_t pin;
    bool state;
} Pre_transfer_info;

esp_err_t tsgl_spi_init(size_t maxlen, spi_host_device_t host, int8_t dma) {
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
    return tsgl_spi_initManual(maxlen, host, dma, miso, mosi, clk);
}

esp_err_t tsgl_spi_initManual(size_t maxlen, spi_host_device_t host, int8_t dma, int8_t miso, int8_t mosi, int8_t clk) {
    switch (dma) {
        case TSGL_DMA:
            dma = SPI_DMA_CH_AUTO;
            break;

        case TSGL_NO_DMA:
            dma = SPI_DMA_DISABLED;
            break;
    }
    spi_bus_config_t buscfg={
        .miso_io_num=miso,
        .mosi_io_num=mosi,
        .sclk_io_num=clk,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz = maxlen
    };
    return spi_bus_initialize(host, &buscfg, dma);
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
    ESP_ERROR_CHECK(spi_device_polling_transmit(*((spi_device_handle_t*)display->interface), &t));
}

void tsgl_spi_sendData(tsgl_display* display, const uint8_t* data, size_t size) {
    if (size == 0) return;
    Pre_transfer_info pre_transfer_info = {
        .pin = display->dc,
        .state = true
    };
    spi_transaction_t t = {
        .length = size * 8,
        .tx_buffer = data,
        .user = (void*)(&pre_transfer_info)
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(*((spi_device_handle_t*)display->interface), &t));
}

void tsgl_spi_pre_transfer_callback(spi_transaction_t* t) {
    Pre_transfer_info* pre_transfer_info = (Pre_transfer_info*)t->user;
    gpio_set_level(pre_transfer_info->pin, pre_transfer_info->state);
}