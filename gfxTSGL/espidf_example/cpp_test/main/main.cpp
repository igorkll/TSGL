#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wnarrowing"
#pragma GCC diagnostic push

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <TSGL.hpp>
#include <TSGL_drivers/st7789.h>
#include <esp_timer.h>

#define DC 21
#define CS 22
#define RST 18

tsgl_driver_settings driverSettings = {
    .width = 240,
    .height = 320
};

TSGL_Display display;

float fmap(float value, float low, float high, float low_2, float high_2) {
    float relative_value = (value - low) / (high - low);
    float scaled_value = low_2 + (high_2 - low_2) * relative_value;
    return scaled_value;
}

int imap(int value, int low, int high, int low_2, int high_2) {
    return (int)(fmap(value, low, high, low_2, high_2) + 0.5);
}

extern "C" void app_main() {
    display.begin(&st7789_rgb565, driverSettings, true, TSGL_HOST1, 60000000, DC, CS, RST);

    while (true) {
        display.clear(TSGL_GREEN);
        display.update();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        display.clear(TSGL_YELLOW);
        display.update();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        display.clear(TSGL_BLUE);
        display.update();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        display.clear(TSGL_PURPLE);
        display.update();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        for (uint8_t i = 0; i < 4; i++) {
            display.setRotation(i);
            display.clear(TSGL_BROWN);
            display.fill(10, 10, 25, 25, TSGL_LIME);
            display.rect(10, 10, display.width - 20, display.height - 20, TSGL_RED, 10);
            display.update();
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        for (uint8_t i = 0; i < 4; i++) {
            display.setRotation(i);
            for (tsgl_pos pos = 0; pos < display.width; pos++) {
                display.fill(pos, 0, 1, display.height, tsgl_color_hsv(fmap(pos, 0, display.width - 1, 0, 255), 255, 255));
            }
            display.update();
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        for (uint8_t i = 0; i < 4; i++) {
            display.setRotation(i);
            for (tsgl_pos posx = 0; posx < display.width; posx++) {
                for (tsgl_pos posy = 0; posy < display.height; posy++) {
                    display.set(posx, posy, tsgl_color_hsv(fmap(posx + posy, 0, (display.width - 1) + (display.height - 1), 0, 255), 255, 255));
                }
            }
            display.update();
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        display.setRotation(1);
        for (uint8_t i = 0; i < 255;) {
            int64_t t1 = esp_timer_get_time();
            for (tsgl_pos pos = 0; pos < display.width; pos++) {
                display.fill(pos, 0, 1, display.height, tsgl_color_hsv((imap(pos, 0, display.width - 1, 0, 255) - i) % 256, 255, 255));
            }
            int64_t t2 = esp_timer_get_time();
            display.update();
            i += ((t2 - t1) / 1000000) * 64;
        }
    }
}