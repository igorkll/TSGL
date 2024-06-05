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

bool tsgl_display_spi(tsgl_display* display, tsgl_pos width, tsgl_pos height, spi_host_device_t spihost, size_t freq, int8_t dc, int8_t cs, int8_t rst) {
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

    return spi_bus_add_device(spihost, &devcfg, (spi_device_handle_t*)display->interface) == ESP_OK;
}

bool tsgl_display_initSpi(tsgl_display* display, tsgl_pos width, tsgl_pos height, spi_host_device_t spihost, size_t freq, int8_t dc, int8_t cs, int8_t rst, int8_t miso, int8_t mosi, int8_t clk) {
    spi_bus_config_t buscfg={
        .miso_io_num=miso,
        .mosi_io_num=mosi,
        .sclk_io_num=clk,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz = width * height * 3
    };
    ESP_ERROR_CHECK(spi_bus_initialize(spihost, &buscfg, SPI_DMA_CH_AUTO));
    return tsgl_display_spi(display, width, height, spihost, freq, dc, cs, rst);
}

void tsgl_display_select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos sx, tsgl_pos sy) {
    
}

void tsgl_display_all(tsgl_display* display) {

}

void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            tsgl_spi_sendData(display, framebuffer->buffer, framebuffer->buffersize);
            break;
    }
}

void tsgl_display_free(tsgl_display* display) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            spi_bus_remove_device(*((spi_device_handle_t*)display->interface));
            break;
    }
    free(display->interface);
}