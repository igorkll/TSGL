#include <../../../TSGL_framebuffer.h>
#include <../../../TSGL_color.h>
#include <stdio.h>

#define WIDTH      320
#define HEIGHT     240
#define COLORMODE  tsgl_bgr_565_be

tsgl_framebuffer framebuffer;

void app_main() {
    if (!tsgl_framebuffer_init(&framebuffer, COLORMODE, WIDTH, HEIGHT)) {
        printf("the problem when creating a framebuffer\n");
        while (true);
    }
    tsgl_framebuffer_rotate(&framebuffer, 1); //set rotation

    while (true) {
        tsgl_framebuffer_set(&framebuffer, 1, 1, tsgl_color_pack(255, 0, 0));
    }

    tsgl_framebuffer_free(&framebuffer); //if you don't need this framebuffer anymore, then you should unload it.
}