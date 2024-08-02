#include "TSGL.h"
#include "TSGL_spi.h"
#include "TSGL_display.h"
#include "TSGL_math.h"

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

esp_err_t tsgl_spi_initManual(size_t maxlen, spi_host_device_t host, gpio_num_t mosi, gpio_num_t miso, gpio_num_t clk) {
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