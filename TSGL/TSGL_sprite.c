#include "TSGL_sprite.h"
#include "TSGL_framebuffer.h"

void tsgl_sprite_apply_grayscale(tsgl_sprite* sprite) {
    tsgl_framebuffer* fb = sprite->fb;

    for (size_t ix = 0; ix < fb->width; ix++) {
        for (size_t iy = 0; iy < fb->height; iy++) {
            tsgl_rawcolor sourceRawColor = tsgl_framebuffer_get(fb, ix, iy);
            if (!tsgl_color_rawColorCompare(sourceRawColor, sprite->transparentColor, sprite->fb->colorsize, sprite->fb->floatColorsize)) {
                tsgl_color color = tsgl_color_uraw(sourceRawColor, fb->colormode);
                color = tsgl_color_grayscale(color);
                tsgl_rawcolor rawColor = tsgl_color_raw(color, fb->colormode);            
                tsgl_framebuffer_set(fb, ix, iy, rawColor);
            }
        }
    }
}

void tsgl_sprite_free(tsgl_sprite* sprite) {
    tsgl_framebuffer_free(sprite->fb);
    free(sprite->fb);
    free(sprite);
}
