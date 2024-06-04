#include <../../../TSGL.h>
#include <../../../TSGL_framebuffer.h>
#include <../../../TSGL_display.h>
#include <../../../TSGL_color.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define umax(a,b) (((a) > (b)) ? (a) : (b))
#define umin(a,b) (((a) < (b)) ? (a) : (b))

#define WIDTH      320
#define HEIGHT     240
#define COLORMODE  tsgl_framebuffer_rgb565_be

tsgl_framebuffer framebuffer;
tsgl_display display;

static float fmap(float value, float low, float high, float low_2, float high_2) {
    float relative_value = (value - low) / (high - low);
    float scaled_value = low_2 + (high_2 - low_2) * relative_value;
    return scaled_value;
}

void app_main() {
    assert(tsgl_framebuffer_init(&framebuffer, COLORMODE, WIDTH, HEIGHT));
    assert(tsgl_display_init(&display, WIDTH, HEIGHT));
    tsgl_framebuffer_rotate(&framebuffer, 1); //set rotation
    tsgl_framebuffer_clear(&framebuffer, TSGL_BROWN);

    uint16_t step = 0;
    uint16_t stepMax = umin(framebuffer.width, framebuffer.height) / 2;
    while (true) {
        tsgl_color current = tsgl_color_combine(fmap(step, 0, stepMax, 0, 1), TSGL_CYAN, TSGL_RED);
        tsgl_framebuffer_rect(&framebuffer, step, step, framebuffer.width - (step * 2), framebuffer.height - (step * 2), current);
        tsgl_display_send(&display, &framebuffer);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        step++;
        if (step > stepMax) {
            step = 0;
            for (tsgl_pos i = 0; i < framebuffer.width; i++) {
                tsgl_framebuffer_fill(&framebuffer, i, 0, 1, framebuffer.height, color_hsv(fmap(i, 0, framebuffer.width - 1, 0, 255), 255, 255));
            }
        }
    }

    tsgl_framebuffer_free(&framebuffer); //if you don't need this framebuffer anymore, then you should unload it.
    tsgl_display_free(&display);
}