#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>

typedef struct {
    int32_t width;
    int32_t height;
    uint8_t bits;
    bool reverseLines;
} ImageInfo;

typedef struct {
    uint16_t* img;
    int32_t width;
    int32_t height;
} Image;

#define BMP_BUFFER_SIZE (2 * 1024)

ImageInfo bmp_readImageInfo(const char* path);
Image* bmp_load(const char* path);