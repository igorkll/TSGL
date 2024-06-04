#include <../../../TSGL_framebuffer.h>
#include <stdio.h>

#define WIDTH      320
#define HEIGHT     240
#define COLORMODE  tsgl_rgb_565

tsgl_framebuffer framebuffer;

void app_main() {
    if (!tsgl_framebuffer_init(&framebuffer, COLORMODE, WIDTH, HEIGHT)) {
        printf("the problem when creating a framebuffer\n");
        while (true);
    }

    while (true) {
    }

    tsgl_framebuffer_free(&framebuffer); //if you don't need this framebuffer anymore, then you should unload it.
}