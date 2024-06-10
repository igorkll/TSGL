#pragma once
#include "TSGL.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"
#include "TSGL_color.h"
#include "TSGL_spi.h"
#include <esp_heap_caps.h>

class TSGL_Display {
    public:
    tsgl_display display;
    tsgl_framebuffer* framebuffer = NULL;
    tsgl_pos& width = display.width;
    tsgl_pos& height = display.height;

    TSGL_Display (const tsgl_driver* driver, const tsgl_driver_settings driver_settings, bool buffered, spi_host_device_t spihost, size_t freq, int8_t dc, int8_t cs, int8_t rst) {
        //without checking because the SPI may already be initialized
        tsgl_spi_init(driver_settings.width * driver_settings.height * tsgl_colormodeSizes[driver->colormode], spihost);
        if (buffered) {
            //attempt to allocate a buffer in external memory
            ESP_ERROR_CHECK(tsgl_framebuffer_init(framebuffer, driver->colormode, driver_settings.width, driver_settings.height, MALLOC_CAP_SPIRAM));
        }
        ESP_ERROR_CHECK(tsgl_display_spi(&display, &driver, driver_settings, spihost, freq, dc, cs, rst));
    }

    TSGL_Display (const tsgl_driver* driver, const tsgl_driver_settings driver_settings, bool buffered, spi_host_device_t spihost, size_t freq, int8_t mosi, int8_t miso, int8_t clk, int8_t dc, int8_t cs, int8_t rst) {
        tsgl_spi_initManual(driver_settings.width * driver_settings.height * tsgl_colormodeSizes[driver->colormode], spihost, mosi, miso, clk);
        if (buffered) {
            ESP_ERROR_CHECK(tsgl_framebuffer_init(framebuffer, driver->colormode, driver_settings.width, driver_settings.height, MALLOC_CAP_SPIRAM));
        }
        ESP_ERROR_CHECK(tsgl_display_spi(&display, &driver, driver_settings, spihost, freq, dc, cs, rst));
    }

    ~TSGL_Display () {
        tsgl_display_free(&display);
        tsgl_framebuffer_free(framebuffer);
        free(framebuffer);
    }

    void setInvert(bool state) {
        tsgl_display_setInvert(&display, state);
    }

    void setEnable(bool state) {
        tsgl_display_setEnable(&display, state);
    }

    void setRotation(uint8_t rotation) {
        if (framebuffer == NULL) {
            tsgl_display_rotate(&display, rotation);
        } else {
            tsgl_framebuffer_hardwareRotate(framebuffer, rotation);
            tsgl_display_rotate(&display, rotation);
        }
    }

    void update() {
        if (framebuffer != NULL) tsgl_display_send(&display, framebuffer);
    }

    // --------------------- graphic

    void set(tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
        if (framebuffer == NULL) {
            tsgl_display_set(&display, x, y, color);
        } else {
            tsgl_framebuffer_set(framebuffer, x, y, color);
        }
    }

    void fill(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
        if (framebuffer == NULL) {
            tsgl_display_fill(&display, x, y, width, height, color);
        } else {
            tsgl_framebuffer_fill(framebuffer, x, y, width, height, color);
        }
    }
    
    void rect(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos strokelen) {
        if (framebuffer == NULL) {
            tsgl_display_rect(&display, x, y, width, height, color, strokelen);
        } else {
            tsgl_framebuffer_rect(framebuffer, x, y, width, height, color, strokelen);
        }
    }

    void clear(tsgl_rawcolor color) {
        if (framebuffer == NULL) {
            tsgl_display_clear(&display, color);
        } else {
            tsgl_framebuffer_clear(framebuffer, color);
        }
    }
    
    tsgl_rawcolor get(tsgl_pos x, tsgl_pos y) {
        if (framebuffer == NULL) {
            return TSGL_BLACK; //temporarily unavailable
        } else {
            return tsgl_framebuffer_get(framebuffer, x, y);
        }
    }

    // --------------------- high-level color API

    void set(tsgl_pos x, tsgl_pos y, tsgl_color color) {
        set(x, y, tsgl_color_raw(color, display.colormode));
    }

    void fill(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_color color) {
        fill(x, y, width, height, tsgl_color_raw(color, display.colormode));
    }
    
    void rect(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_color color, tsgl_pos strokelen) {
        rect(x, y, width, height, tsgl_color_raw(color, display.colormode), strokelen);
    }

    void clear(tsgl_color color) {
        clear(tsgl_color_raw(color, display.colormode));
    }
    
    tsgl_color get(tsgl_pos x, tsgl_pos y) {
        return tsgl_color_uraw(get(x, y), display.colormode);
    }
};