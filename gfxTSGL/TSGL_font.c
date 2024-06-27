#include "TSGL.h"

bool tsgl_font_isSmoothing(void* font) {
    return ((uint8_t*)font)[0] > 0;
}

size_t tsgl_font_find(void* font, char chr) {
    uint8_t* ptr = font;
    size_t index = 1;
    bool smoothing = tsgl_font_isSmoothing(font);
}

size_t tsgl_font_width(void* font, char chr) {
    
}

size_t tsgl_font_height(void* font, char chr) {
    
}

size_t tsgl_font_size(void* font, char chr) {
    return tsgl_font_width(font, chr) * tsgl_font_height(font, chr);
}