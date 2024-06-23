// -------------------------------- display settings
#include <TSGL_drivers/st77XX.h>

#define SPI  TSGL_HOST1
#define FREQ 60000000
#define BUFFER TSGL_SPIRAM
#define DC 21
#define CS 22
#define RST 18

const tsgl_settings settings = {
    .driver = &st77XX_rgb565,
    .invert = true,
    .spawRGB = true,
    .flipX = true,
    .width = 320,
    .height = 480
};

// --------------------------------
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_log.h>

#include <TSGL.h>
#include <TSGL_framebuffer.h>
#include <TSGL_display.h>
#include <TSGL_color.h>
#include <TSGL_spi.h>

tsgl_display display;
tsgl_framebuffer framebuffer;

float fmap(float value, float low, float high, float low_2, float high_2) {
    float relative_value = (value - low) / (high - low);
    float scaled_value = low_2 + (high_2 - low_2) * relative_value;
    return scaled_value;
}

int imap(int value, int low, int high, int low_2, int high_2) {
    return (int)(fmap(value, low, high, low_2, high_2) + 0.5);
}

void app_main() {
    tsgl_colormode colormode = settings.driver->colormode;
    ESP_ERROR_CHECK(tsgl_spi_init(settings.width * settings.height * tsgl_colormodeSizes[colormode], SPI));
    tsgl_display_pushInitColor(tsgl_color_raw(TSGL_RED, tsgl_display_reColormode(settings, colormode)));
    ESP_ERROR_CHECK(tsgl_display_spi(&display, settings, SPI, FREQ, DC, CS, RST));
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer, colormode, settings.width, settings.height, BUFFER));

    while (true) {
        tsgl_framebuffer_clear(&framebuffer, display.black);
        tsgl_framebuffer_line(&framebuffer, 0, 0, display.width, display.height, tsgl_color_raw(TSGL_GREEN, colormode), 5);
        tsgl_display_send(&display, &framebuffer);
        vTaskDelay(1);
    }
}