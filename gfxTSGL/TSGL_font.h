#include "TSGL.h"

bool tsgl_font_isSmoothing(void* font);
size_t tsgl_font_find(void* font, char chr);
size_t tsgl_font_width(void* font, char chr);
size_t tsgl_font_height(void* font, char chr);
size_t tsgl_font_size(void* font, char chr);
uint8_t tsgl_font_parse(void* font, size_t lptr, size_t index);