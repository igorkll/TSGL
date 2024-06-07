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



tsgl_framebuffer framebuffer;
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
    for (tsgl_pos i = 0; i < framebuffer.width; i++) {
        tsgl_framebuffer_fill(&framebuffer, i, 0, 1, framebuffer.height, tsgl_color_raw(tsgl_color_hsv(fmap(i, 0, framebuffer.width - 1, 0, 255), 255, 255), COLORMODE));
    }
}

void colorBox(tsgl_pos y, tsgl_color color) {
    int boxSize = framebuffer.width / 32;
    tsgl_framebuffer_fill(&framebuffer, 0, (y * boxSize) + boxSize, framebuffer.width / 2, boxSize, tsgl_color_raw(color, COLORMODE));
}

void printFreeRamSize(const char* title) {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    printf("%s. free ram: %f\n", title, info.total_free_bytes / 1024.0);
}

void app_main() {
    printFreeRamSize("before display init");
    ESP_ERROR_CHECK(tsgl_spi_init(WIDTH * HEIGHT * tsgl_colormodeSizes[COLORMODE], TSGL_HOST1));
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer, COLORMODE, WIDTH, HEIGHT, MALLOC_CAP_SPIRAM));
    ESP_ERROR_CHECK(tsgl_display_spi(&display, &DRIVER, WIDTH, HEIGHT, TSGL_HOST1, 60000000, 21, 22, 18));
    printFreeRamSize("after display init");
    hue();

    tsgl_display_clear(&display, tsgl_color_raw(TSGL_RED, COLORMODE));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    tsgl_display_clear(&display, tsgl_color_raw(TSGL_GREEN, COLORMODE));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    tsgl_display_clear(&display, tsgl_color_raw(TSGL_CYAN, COLORMODE));
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    uint16_t step = 0;
    uint16_t stepMax = umin(framebuffer.width, framebuffer.height) / 2;
    uint8_t rotation = 0;
    uint32_t currentFrame = 0;
    uint32_t oldFrame = 0;
    TickType_t oldFPSCheckTime = 0;
    while (true) {
        tsgl_color current = tsgl_color_combine(fmap(step, 0, stepMax, 0, 1), TSGL_RED, TSGL_LIME);
        tsgl_framebuffer_rect(&framebuffer, step, step, framebuffer.width - (step * 2), framebuffer.height - (step * 2), tsgl_color_raw(current, COLORMODE));
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
        tsgl_display_send(&display, &framebuffer);

        step++;
        if (step > stepMax) {
            rotation = (rotation + 1) % 4;
            tsgl_framebuffer_rotate(&framebuffer, rotation); //rotates the indexing of the framebuffer and not the framebuffer itself

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

    tsgl_framebuffer_free(&framebuffer); //if you don't need this framebuffer anymore, then you should unload it.
    tsgl_display_free(&display);
}