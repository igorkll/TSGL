#pragma once
extern "C" {
    #include "TSGL.h"
    #include "TSGL_framebuffer.h"
    #include "TSGL_display.h"
    #include "TSGL_color.h"
    #include "TSGL_spi.h"
}

#define TSGL_NOBUFFER MALLOC_CAP_INVALID

class TSGL_Display {
    public:
    tsgl_display display;
    tsgl_framebuffer* framebuffer = NULL;
    tsgl_pos& width = display.width;
    tsgl_pos& height = display.height;
    tsgl_colormode& colormode = display.colormode;
    float& colorsize = display.colorsize;

    void begin(const tsgl_driver* driver, const tsgl_driver_settings driver_settings, int64_t caps, spi_host_device_t spihost, size_t freq, int8_t dc, int8_t cs, int8_t rst) {
        //without checking because the SPI may already be initialized
        tsgl_spi_init(driver_settings.width * driver_settings.height * tsgl_colormodeSizes[driver->colormode], spihost);
        if (caps != MALLOC_CAP_INVALID) {
            framebuffer = (tsgl_framebuffer*)malloc(sizeof(tsgl_framebuffer));
            if (tsgl_framebuffer_init(framebuffer, driver->colormode, driver_settings.width, driver_settings.height, caps) != ESP_OK) {
                ::free(framebuffer);
                framebuffer = NULL;
            }
        }
        ESP_ERROR_CHECK(tsgl_display_spi(&display, driver, driver_settings, spihost, freq, dc, cs, rst));
    }

    void begin(const tsgl_driver* driver, const tsgl_driver_settings driver_settings, int64_t caps, spi_host_device_t spihost, size_t freq, int8_t mosi, int8_t miso, int8_t clk, int8_t dc, int8_t cs, int8_t rst) {
        tsgl_spi_initManual(driver_settings.width * driver_settings.height * tsgl_colormodeSizes[driver->colormode], spihost, mosi, miso, clk);
        if (caps != MALLOC_CAP_INVALID) {
            framebuffer = (tsgl_framebuffer*)malloc(sizeof(tsgl_framebuffer));
            if (tsgl_framebuffer_init(framebuffer, driver->colormode, driver_settings.width, driver_settings.height, caps) != ESP_OK) {
                ::free(framebuffer);
                framebuffer = NULL;
            }
        }
        ESP_ERROR_CHECK(tsgl_display_spi(&display, driver, driver_settings, spihost, freq, dc, cs, rst));
    }

    void free() {
        tsgl_display_free(&display);
        tsgl_framebuffer_free(framebuffer);
        ::free(framebuffer);
    }

    ~TSGL_Display () {
        free();
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

    void push(tsgl_pos x, tsgl_pos y, uint8_t rotation, tsgl_framebuffer* sprite) {
        if (framebuffer == NULL) {
            tsgl_display_push(&display, x, y, rotation, sprite);
        } else {
            tsgl_framebuffer_push(framebuffer, x, y, rotation, sprite);
        }
    }

    // --------------------- tsgl_rawcolor graphic

    void set(tsgl_pos x, tsgl_pos y, tsgl_rawcolor color) {
        if (framebuffer == NULL) {
            tsgl_display_set(&display, x, y, color);
        } else {
            tsgl_framebuffer_set(framebuffer, x, y, color);
        }
    }

    void line(tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke) {
        if (framebuffer == NULL) {
            tsgl_display_line(&display, x1, y1, x2, y2, color, stroke);
        } else {
            tsgl_framebuffer_line(framebuffer, x1, y1, x2, y2, color, stroke);
        }
    }

    void line(tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color) {
        line(x1, y1, x2, y2, color, 1);
    }

    void fill(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
        if (framebuffer == NULL) {
            tsgl_display_fill(&display, x, y, width, height, color);
        } else {
            tsgl_framebuffer_fill(framebuffer, x, y, width, height, color);
        }
    }
    
    void rect(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke) {
        if (framebuffer == NULL) {
            tsgl_display_rect(&display, x, y, width, height, color, stroke);
        } else {
            tsgl_framebuffer_rect(framebuffer, x, y, width, height, color, stroke);
        }
    }

    void rect(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color) {
        rect(x, y, width, height, color, 1);
    }

    void clear(tsgl_rawcolor color) {
        if (framebuffer == NULL) {
            tsgl_display_clear(&display, color);
        } else {
            tsgl_framebuffer_clear(framebuffer, color);
        }
    }
    
    tsgl_rawcolor rawGet(tsgl_pos x, tsgl_pos y) {
        if (framebuffer == NULL) {
            return display.black; //temporarily unavailable
        } else {
            return tsgl_framebuffer_get(framebuffer, x, y);
        }
    }

    // --------------------- tsgl_color graphic

    void set(tsgl_pos x, tsgl_pos y, tsgl_color color) {
        set(x, y, tsgl_color_raw(color, display.colormode));
    }

    void line(tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_color color, tsgl_pos stroke) {
        line(x1, y1, x2, y2, tsgl_color_raw(color, display.colormode), stroke);
    }

    void line(tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_color color) {
        line(x1, y1, x2, y2, color, 1);
    }

    void fill(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_color color) {
        fill(x, y, width, height, tsgl_color_raw(color, display.colormode));
    }
    
    void rect(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_color color, tsgl_pos stroke) {
        rect(x, y, width, height, tsgl_color_raw(color, display.colormode), stroke);
    }

    void rect(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_color color) {
        rect(x, y, width, height, color, 1);
    }

    void clear(tsgl_color color) {
        clear(tsgl_color_raw(color, display.colormode));
    }
    
    tsgl_color get(tsgl_pos x, tsgl_pos y) {
        return tsgl_color_uraw(rawGet(x, y), display.colormode);
    }

    // --------------------- tsgl_color graphic

    void set(tsgl_pos x, tsgl_pos y, uint32_t color) {
        set(x, y, tsgl_color_fromHex(color));
    }

    void line(tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, uint32_t color, tsgl_pos stroke) {
        line(x1, y1, x2, y2, tsgl_color_fromHex(color), stroke);
    }

    void line(tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, uint32_t color) {
        line(x1, y1, x2, y2, color, 1);
    }

    void fill(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, uint32_t color) {
        fill(x, y, width, height, tsgl_color_fromHex(color));
    }
    
    void rect(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, uint32_t color, tsgl_pos stroke) {
        rect(x, y, width, height, tsgl_color_fromHex(color), stroke);
    }

    void rect(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, uint32_t color) {
        rect(x, y, width, height, color, 1);
    }

    void clear(uint32_t color) {
        clear(tsgl_color_fromHex(color));
    }
    
    uint32_t hexGet(tsgl_pos x, tsgl_pos y) {
        return tsgl_color_toHex(get(x, y));
    }
};