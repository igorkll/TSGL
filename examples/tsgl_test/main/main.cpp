#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <TSGL.hpp>
#include <TSGL_drivers/st77XX.h>
#include <esp_timer.h>

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
    TSGL_Display::pushInitColor(TSGL_GRAY, settings.driver->colormode);
    display.begin(settings, TSGL_SPIRAM, TSGL_HOST1, 60000000, DC, CS, RST); //TSGL_SPIRAM, TSGL_BUFFER, TSGL_NOBUFFER
    //display.enableAsyncSending(TSGL_NOBUFFER);

    tsgl_framebuffer framebuffer;
    tsgl_framebuffer_init(&framebuffer, display.colormode, 128, 256, TSGL_SPIRAM);
    for (tsgl_pos posx = 0; posx < framebuffer.width; posx++) {
        for (tsgl_pos posy = 0; posy < framebuffer.height; posy++) {
            tsgl_color hue = tsgl_color_hsv(fmap(posx + posy, 0, (framebuffer.width - 1) + (framebuffer.height - 1), 0, 255), 255, 255);
            tsgl_framebuffer_set(&framebuffer, posx, posy, tsgl_color_raw(hue, display.colormode));
        }
    }
    tsgl_framebuffer_fill(&framebuffer, 0, 0, 64, 64, tsgl_color_raw(TSGL_GREEN, display.colormode));

    float waittime;
    while (true) {
        display.setRotation(1);
        display.clear(TSGL_BLACK);
        display.fill(0, 0, 64, 64, TSGL_RED);
        display.fill(64, 0, 64, 64, TSGL_GREEN);
        display.fill(0, 64, 64, 64, TSGL_BLUE);
        display.update();
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        printf("fps test (minimal rendering)\n");
        display.clear(TSGL_BLACK);
        int64_t t1 = esp_timer_get_time();
        for (uint16_t i = 0; i < 256; i++) {
            display.fill(0, 0, 16, 16, tsgl_color_hsv(i, 255, 255));
            display.update();
        }
        int64_t t2 = esp_timer_get_time();
        float exectime = (t2 - t1) / 1000 / 1000 / 256.0;
        printf("execute time: %f\n", exectime);
        printf("FPS: %f\n", exectime == 0 ? -1 : 1.0 / exectime);

        printf("fps test (large rendering)\n");
        t1 = esp_timer_get_time();
        for (tsgl_pos i = 0; i < display.width; i++) {
            uint8_t hue = i % 256;
            display.clear(tsgl_color_hsv(hue, 255, 180));
            tsgl_rawcolor color = tsgl_color_raw(tsgl_color_hsv(255 - hue, 140, 180), display.colormode);
            for (tsgl_pos ix = 0; ix < display.width; ix += 8) {
                display.line(0, 0, ix, display.height, color);
            }
            display.fill(i, 0, 16, display.height, tsgl_color_hsv(255 - hue, 180, 255));
            display.update();
        }
        t2 = esp_timer_get_time();
        exectime = (t2 - t1) / 1000 / 1000.0 / display.width;
        printf("execute time: %f\n", exectime);
        printf("FPS: %f\n", exectime == 0 ? -1 : 1.0 / exectime);

        for (uint8_t i = 0; i < 4; i++) {
            display.clear(TSGL_WHITE);
            display.fill(0, 0, 16, 16, TSGL_RED);
            display.push(32, 32, i, &framebuffer);
            display.update();
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        waittime = 10;
        for (uint8_t i = 0; i < 2; i++) {
            for (tsgl_pos pos = 0; pos < display.width;) {
                int64_t t1 = esp_timer_get_time();
                display.clear(TSGL_WHITE);
                display.fill(0, 0, 16, 16, TSGL_RED);
                display.push(pos, 18, 0, &framebuffer);
                display.update();
                int64_t t2 = esp_timer_get_time();

                int64_t exectime = (t2 - t1) / 1000;
                float delay = waittime - exectime;
                if (delay > 0) {
                    vTaskDelay(delay / portTICK_PERIOD_MS);
                    pos++;
                } else {
                    pos += exectime / waittime;
                }
            }
        }

        display.setRotation(0);
        display.clear(TSGL_BLACK);
        for (tsgl_pos ix = 0; ix < display.width; ix += 8) {
            display.line(0, 0, ix, display.height, TSGL_RED);
        }
        for (tsgl_pos iy = 0; iy < display.height; iy += 8) {
            display.line(0, 0, display.width, iy, TSGL_GREEN);
        }
        display.line(0, 0, display.width, display.height, TSGL_BLUE, 3);
        display.update();
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        display.clear(TSGL_YELLOW);
        display.update();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        display.clear(TSGL_CYAN);
        display.update();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        display.clear(TSGL_BLUE);
        display.update();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        for (uint8_t i = 0; i < 4; i++) {
            display.setRotation(i);
            display.clear(TSGL_GRAY);
            display.fill(10, 10, 25, 25, TSGL_GREEN);
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
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        waittime = 10;
        display.setRotation(1);
        for (uint16_t i = 0; i < 255 * 2;) {
            int64_t t1 = esp_timer_get_time();
            for (tsgl_pos pos = 0; pos < display.width; pos++) {
                display.fill(pos, 0, 1, display.height, tsgl_color_hsv((imap(pos, 0, display.width - 1, 0, 255) - i) % 256, 255, 255));
            }
            display.update();
            int64_t t2 = esp_timer_get_time();

            int64_t exectime = (t2 - t1) / 1000;
            float delay = waittime - exectime;
            if (delay > 0) {
                vTaskDelay(delay / portTICK_PERIOD_MS);
                i++;
            } else {
                i += exectime / waittime;
            }
        }
    }
}