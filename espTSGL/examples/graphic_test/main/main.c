#include <TSGL.h>
#include <TSGL_framebuffer.h>
#include <TSGL_display.h>
#include <TSGL_color.h>
#include <TSGL_spi.h>

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
#include <esp_err.h>

#define WIDTH       320
#define HEIGHT      240
#define COLORMODE   tsgl_rgb565_be
#define FREQUENCY   60000000
#define DISPLAY_SPI TSGL_HOST1
#define DISPLAY_DC  21
#define DISPLAY_CS  22
#define DISPLAY_RST 18
//#define CUSTOM_SPI_GPIO //when using custom GPIO pins instead of standard ones, the frequency cannot be higher than 26 megahertz (20000000 recommended)
//#define CUSTOM_MISO x
//#define CUSTOM_MOSI x
//#define CUSTOM_CLK x

tsgl_framebuffer framebuffer;
tsgl_display display;

void app_main() {
    #ifdef CUSTOM_SPI_GPIO
        ESP_ERROR_CHECK(tsgl_spi_initManual(WIDTH * HEIGHT * tsgl_colormodeSizes[COLORMODE], TSGL_HOST1, TSGL_DMA, CUSTOM_MISO, CUSTOM_MOSI, CUSTOM_CLK));
    #else
        ESP_ERROR_CHECK(tsgl_spi_init(WIDTH * HEIGHT * tsgl_colormodeSizes[COLORMODE], TSGL_HOST1, TSGL_DMA));
    #endif
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer, COLORMODE, WIDTH, HEIGHT, TSGL_DMA));
    ESP_ERROR_CHECK(tsgl_display_spi(&display, WIDTH, HEIGHT, TSGL_HOST1, FREQUENCY, DISPLAY_DC, DISPLAY_CS, DISPLAY_RST));
    
    tsgl_framebuffer_rotate(&framebuffer, 3); //making the screen vertical
    tsgl_pos margin = 32;
    uint8_t hue = 0;
    while (true) {
        tsgl_framebuffer_clear(&framebuffer, framebuffer.black);
        tsgl_framebuffer_fill(&framebuffer, margin, margin, framebuffer.width - (margin * 2), framebuffer.height - (margin * 2), tsgl_color_raw(tsgl_color_hsv(hue, 255, 255), COLORMODE));
        tsgl_display_send(&display, &framebuffer);
        hue += 4;
    }

    tsgl_framebuffer_free(&framebuffer); //if you don't need this framebuffer anymore, then you should unload it.
    tsgl_display_free(&display);
}