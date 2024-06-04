#include "TSGL.h"
#include "TSGL_display.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"

#include <esp_system.h>
#include <esp_err.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>

#define LCD_HOST    HSPI_HOST

#define PIN_NUM_MISO 12
#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   22

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18

static void lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
    gpio_set_level(PIN_NUM_DC, (bool)t->user);
}

void tsgl_display_init(tsgl_display* display, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height) {
    spi_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=width * height * tsgl_colormode_sizes[colormode]
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=40*1000*1000,
        .mode=0,
        .spics_io_num=PIN_NUM_CS,
        .queue_size=7,
        .pre_cb=lcd_spi_pre_transfer_callback,
    };
    display->spi = malloc(sizeof(spi_device_handle_t));
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &devcfg, *display->spi));
}

void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer) {

}

void tsgl_display_free(tsgl_display* display) {
    if (display->spi != NULL) {
        spi_bus_remove_device(*display->spi);
        free(display->spi);
    }
}