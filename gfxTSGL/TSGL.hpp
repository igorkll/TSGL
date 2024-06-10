#pragma once
#include "TSGL.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"
#include "TSGL_color.h"
#include "TSGL_spi.h"

class TSGL_Display {
    public:
    tsgl_display display;
    tsgl_framebuffer* framebuffer;

    TSGL_Display (bool buffered, const tsgl_driver* driver, const tsgl_driver_settings driver_settings, spi_host_device_t spihost, size_t freq, int8_t dc, int8_t cs, int8_t rst) {
        tsgl_spi_init(driver_settings.width * driver_settings.height * tsgl_colormodeSizes[driver->colormode], spihost); //without checking because the SPI may already be initialized
        if (buffered) {
            ESP_ERROR_CHECK(tsgl_framebuffer_init(framebuffer, COLORMODE, WIDTH, HEIGHT, MALLOC_CAP_SPIRAM)); //attempt to allocate a buffer in external memory
        }
        ESP_ERROR_CHECK(tsgl_display_spi(&display, &driver, driver_settings, spihost, freq, dc, cs, rst));
    }

    ~TSGL_Display () {
        tsgl_display_free(&display);
        tsgl_framebuffer_free(framebuffer);
        free(framebuffer);
    }
};