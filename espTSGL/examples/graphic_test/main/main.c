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
#define COLORMODE  tsgl_rgb_565_be

tsgl_framebuffer framebuffer;
tsgl_display display;

static float fmap(float value, float low, float high, float low_2, float high_2) {
    float relative_value = (value - low) / (high - low);
    float scaled_value = low_2 + (high_2 - low_2) * relative_value;
    return scaled_value;
}

void app_main() {
    tsgl_color bg = tsgl_color_pack(0, 16, 64);
    tsgl_color first = tsgl_color_pack(0, 255, 0);
    tsgl_color last = tsgl_color_pack(255, 0, 0);

    assert(tsgl_framebuffer_init(&framebuffer, COLORMODE, WIDTH, HEIGHT));
    assert(tsgl_display_init(&display, COLORMODE, WIDTH, HEIGHT));
    tsgl_framebuffer_rotate(&framebuffer, 0); //set rotation
    tsgl_framebuffer_clear(&framebuffer, bg);

    uint16_t step = 0;
    uint16_t stepMax = umin(framebuffer.width, framebuffer.height) / 2;
    while (true) {
        tsgl_color current = tsgl_color_combine(fmap(step, 0, stepMax, -1, 1), first, last);
        tsgl_framebuffer_rect(&framebuffer, step, step, framebuffer.width - (step * 2) - 1, framebuffer.height - (step * 2) - 1, current);
        tsgl_display_send(&display, &framebuffer);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        step++;
        if (step > stepMax) {
            step = 0;
            tsgl_framebuffer_clear(&framebuffer, bg);
        }
    }

    tsgl_framebuffer_free(&framebuffer); //if you don't need this framebuffer anymore, then you should unload it.
    tsgl_display_free(&display);
}