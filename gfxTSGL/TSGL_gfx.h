#pragma once
#include "TSGL.h"
#include "TSGL_color.h"

#define TSGL_GFX_RECT(arg, fill, x, y, width, height, color, strokelen) \
fill(arg, x, y, width, strokelen, color); \
fill(arg, x, (y + height) - strokelen, width, strokelen, color); \
fill(arg, x, y + 1, strokelen, height - 2, color); \
fill(arg, (x + width) - strokelen, y + 1, strokelen, height - 2, color);