#include "TSGL.h"

bool tsgl_font_isSmoothing(const void* font);
size_t tsgl_font_find(const void* font, char chr);
size_t tsgl_font_width(const void* font, char chr);
size_t tsgl_font_height(const void* font, char chr);
uint8_t tsgl_font_parse(const void* font, size_t lptr, size_t index);