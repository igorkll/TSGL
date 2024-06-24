// -------------------------------- display settings
#include <TSGL_drivers/st77XX.h>

#define SPI  TSGL_HOST1
#define FREQ 60000000
#define BUFFER TSGL_SPIRAM
#define DC 21
#define CS 22
#define RST 18
#define BL 5

const tsgl_settings settings = {
    .driver = &st77XX_rgb565,
    .swapRGB = true,
    .invert = true,
    .flipX = true,
    .width = 320,
    .height = 480
};

// --------------------------------
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <math.h>

#include <TSGL.h>
#include <TSGL_benchmark.h>
#include <TSGL_framebuffer.h>
#include <TSGL_display.h>
#include <TSGL_color.h>
#include <TSGL_spi.h>

tsgl_display display;
tsgl_framebuffer framebuffer;
tsgl_benchmark benchmark;

float fmap(float value, float low, float high, float low_2, float high_2) {
    float relative_value = (value - low) / (high - low);
    float scaled_value = low_2 + (high_2 - low_2) * relative_value;
    return scaled_value;
}

int imap(int value, int low, int high, int low_2, int high_2) {
    return (int)(fmap(value, low, high, low_2, high_2) + 0.5);
}

void delay(int time) {
    vTaskDelay(time / portTICK_PERIOD_MS);
}

void app_main() {
    ESP_ERROR_CHECK(tsgl_spi_init(settings.width * settings.height * tsgl_colormodeSizes[settings.driver->colormode], SPI));
    tsgl_display_pushInitColor(tsgl_color_raw(TSGL_RED, settings.driver->colormode));
    ESP_ERROR_CHECK(tsgl_display_spi(&display, settings, SPI, FREQ, DC, CS, RST));
    ESP_ERROR_CHECK_WITHOUT_ABORT(tsgl_display_attachBacklight(&display, BL, 255));
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer, display.colormode, settings.width, settings.height, BUFFER));

    tsgl_framebuffer_hardwareRotate(&framebuffer, 1);
    tsgl_display_rotate(&display, 1);

    tsgl_rawcolor blue = tsgl_color_raw(TSGL_BLUE, framebuffer.colormode);
    tsgl_rawcolor yellow = tsgl_color_raw(TSGL_YELLOW, framebuffer.colormode);
    tsgl_rawcolor red = tsgl_color_raw(TSGL_RED, framebuffer.colormode);
    tsgl_pos center = framebuffer.width / 2;
    tsgl_pos sinSize = framebuffer.width / 4;

    while (true) {
        tsgl_framebuffer_clear(&framebuffer, display.black);
        tsgl_framebuffer_line(&framebuffer, 0, 0, display.width, 0, tsgl_color_raw(TSGL_RED, framebuffer.colormode), 5);
        tsgl_framebuffer_line(&framebuffer, 0, 0, display.width, display.height, tsgl_color_raw(TSGL_GREEN, framebuffer.colormode), 5);
        tsgl_framebuffer_line(&framebuffer, 0, 0, 0, display.height, tsgl_color_raw(TSGL_BLUE, framebuffer.colormode), 5);
        tsgl_display_send(&display, &framebuffer);
        delay(3000);

        for (tsgl_pos i = 0; i < display.width; i += tsgl_benchmark_processMulInt(&benchmark, 30)) {
            tsgl_benchmark_startRendering(&benchmark);
            tsgl_framebuffer_clear(&framebuffer, display.black);
            float sinValue = 0;
            tsgl_pos oldY = -1;
            for (tsgl_pos pos = 0; pos < display.width; pos++) {
                float lsin = sin(fmap((pos - i) % sinSize, 0, sinSize, 0, M_PI * 2));
                uint16_t y = (display.height / 2) - (lsin * display.height * 0.4);
                if (pos == center) sinValue = lsin;
                if (oldY >= 0) {
                    tsgl_framebuffer_line(&framebuffer, pos, y, pos, oldY, yellow, 1);
                }
                tsgl_framebuffer_set(&framebuffer, pos, y, yellow);
                oldY = y;
            }
            tsgl_framebuffer_line(&framebuffer, i, 0, i, framebuffer.height - 1, blue, 1);
            tsgl_framebuffer_line(&framebuffer, center, 0, center, framebuffer.height - 1, red, 1);
            tsgl_benchmark_endRendering(&benchmark);

            tsgl_benchmark_startSend(&benchmark);
            tsgl_display_setBacklight(&display, fmap(sinValue, -1, 1, 64, 255));
            tsgl_display_send(&display, &framebuffer);
            tsgl_benchmark_endSend(&benchmark);
            tsgl_benchmark_print(&benchmark);
        }
        tsgl_display_setBacklight(&display, 255);
    }
}