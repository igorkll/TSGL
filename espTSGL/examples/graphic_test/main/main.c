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

void app_main() {
    assert(tsgl_framebuffer_init(&framebuffer, COLORMODE, WIDTH, HEIGHT));
    assert(tsgl_display_init(&display, COLORMODE, WIDTH, HEIGHT));
    tsgl_framebuffer_rotate(&framebuffer, 1); //set rotation

    while (true) {
        tsgl_framebuffer_clear(&framebuffer, tsgl_color_pack(0, 16, 64));
        for (tsgl_pos i = 0; i < umin(framebuffer.width, framebuffer.height); i++) {
            tsgl_framebuffer_set(&framebuffer, i, i, tsgl_color_pack(255, 0, 0));
        }
        tsgl_display_send(&display, &framebuffer);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    tsgl_framebuffer_free(&framebuffer); //if you don't need this framebuffer anymore, then you should unload it.
    tsgl_display_free(&display);
}