#include "TSGL.h"
#include "TSGL_gfx.h"

typedef struct {
    int32_t width;
    int32_t height;
    uint8_t bits;
    bool reverseLines;
} tsgl_imageInfo;

tsgl_imageInfo tsgl_bmp_readImageInfo(const char* path);
tsgl_sprite* tsgl_bmp_load(const char* path, tsgl_colormode colormode, int64_t caps);