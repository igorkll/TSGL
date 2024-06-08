#include <TSGL.h>
#include <TSGL_framebuffer.h>
#include <TSGL_display.h>
#include <TSGL_color.h>
#include <TSGL_spi.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>

#include <TSGL_drivers/st7789.h>

#define WIDTH     320
#define HEIGHT    240
#define DRIVER    st7789_rgb565
#define WITHOUT_FRAMEBUFFER


#ifdef WITHOUT_FRAMEBUFFER
    #define currentWidth display.width
    #define currentHeight display.height
    #define _set(...) tsgl_display_set(&display, __VA_ARGS__)
    #define _fill(...) tsgl_display_fill(&display, __VA_ARGS__)
    #define _rect(...) tsgl_display_rect(&display, __VA_ARGS__)
    #define _clear(...) tsgl_display_clear(&display, __VA_ARGS__)
    #define _rotate(...) tsgl_display_rotate(&display, __VA_ARGS__)
#else
    tsgl_framebuffer framebuffer;
    #define currentWidth framebuffer.width
    #define currentHeight framebuffer.height
    #define _set(...) tsgl_framebuffer_set(&framebuffer, __VA_ARGS__)
    #define _fill(...) tsgl_framebuffer_fill(&framebuffer, __VA_ARGS__)
    #define _rect(...) tsgl_framebuffer_rect(&framebuffer, __VA_ARGS__)
    #define _clear(...) tsgl_framebuffer_clear(&framebuffer, __VA_ARGS__)
    #define _rotate(...) tsgl_framebuffer_rotate(&framebuffer, __VA_ARGS__)
#endif
tsgl_display display;

#define COLORMODE DRIVER.colormode
#define umax(a,b) (((a) > (b)) ? (a) : (b))
#define umin(a,b) (((a) < (b)) ? (a) : (b))

float fmap(float value, float low, float high, float low_2, float high_2) {
    float relative_value = (value - low) / (high - low);
    float scaled_value = low_2 + (high_2 - low_2) * relative_value;
    return scaled_value;
}

void hue() {
    for (tsgl_pos i = 0; i < currentWidth; i++) {
        _fill(i, 0, 1, currentHeight, tsgl_color_raw(tsgl_color_hsv(fmap(i, 0, currentWidth - 1, 0, 255), 255, 255), COLORMODE));
    }
}

void colorBox(tsgl_pos y, tsgl_color color) {
    int boxSize = currentWidth / 32;
    _fill(0, (y * boxSize) + boxSize, currentWidth / 2, boxSize, tsgl_color_raw(color, COLORMODE));
}

void printFreeRamSize(const char* title) {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    printf("%s. free ram: %f\n", title, info.total_free_bytes / 1024.0);
}

void app_main() {
    printFreeRamSize("before display init");
    ESP_ERROR_CHECK(tsgl_spi_init(WIDTH * HEIGHT * tsgl_colormodeSizes[COLORMODE], TSGL_HOST1));
    #ifndef WITHOUT_FRAMEBUFFER
        ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer, COLORMODE, WIDTH, HEIGHT, MALLOC_CAP_SPIRAM));
    #endif
    ESP_ERROR_CHECK(tsgl_display_spi(&display, &DRIVER, WIDTH, HEIGHT, TSGL_HOST1, 60000000, 21, 22, 18));
    printFreeRamSize("after display init");

    // drawing without buffer
    tsgl_display_clear(&display, tsgl_color_raw(TSGL_RED, COLORMODE));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    tsgl_display_clear(&display, tsgl_color_raw(TSGL_GREEN, COLORMODE));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    tsgl_display_clear(&display, tsgl_color_raw(TSGL_CYAN, COLORMODE));
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    _rotate(3);
    hue();
    uint16_t step = 0;
    uint16_t stepMax = umin(currentWidth, currentHeight) / 2;
    uint8_t rotation = 0;
    uint32_t currentFrame = 0;
    uint32_t oldFrame = 0;
    TickType_t oldFPSCheckTime = 0;
    while (true) {
        tsgl_color current = tsgl_color_combine(fmap(step, 0, stepMax, 0, 1), TSGL_RED, TSGL_LIME);
        _rect(step, step, currentWidth - (step * 2), currentHeight - (step * 2), tsgl_color_raw(current, COLORMODE));
        colorBox(0, TSGL_WHITE);
        colorBox(1, TSGL_ORANGE);
        colorBox(2, TSGL_MAGENTA);
        colorBox(3, TSGL_LIGHT_BLUE);
        colorBox(4, TSGL_YELLOW);
        colorBox(5, TSGL_LIME);
        colorBox(6, TSGL_PINK);
        colorBox(7, TSGL_GRAY);
        colorBox(8, TSGL_LIGHT_GRAY);
        colorBox(9, TSGL_CYAN);
        colorBox(10, TSGL_PURPLE);
        colorBox(11, TSGL_BLUE);
        colorBox(12, TSGL_BROWN);
        colorBox(13, TSGL_GREEN);
        colorBox(14, TSGL_RED);
        colorBox(15, TSGL_BLACK);
        #ifndef WITHOUT_FRAMEBUFFER
            tsgl_display_send(&display, &framebuffer);
        #endif

        step++;
        if (step > stepMax) {
            rotation = (rotation + 1) % 4;
            _rotate(rotation);

            step = 0;
            hue();
        }
        
        currentFrame++;
        TickType_t t = xTaskGetTickCount();
        if (t - oldFPSCheckTime > 1000 / portTICK_PERIOD_MS) {
            printf("FPS %li\n", currentFrame - oldFrame);
            oldFrame = currentFrame;
            oldFPSCheckTime = t;
        }

        //vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    #ifndef WITHOUT_FRAMEBUFFER
        tsgl_framebuffer_free(&framebuffer); //if you don't need this framebuffer anymore, then you should unload it.
    #endif
    tsgl_display_free(&display);
}