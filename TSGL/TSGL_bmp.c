#include "TSGL_bmp.h"
#include <TSGL_filesystem.h>
#include <esp_log.h>
#include <string.h>

#define BMP_BUFFER_SIZE (8 * 1024)
static const char* TAG = "TSGL_bmp";

#pragma pack(push, 1)

typedef struct {
    char bfTypeB;
    char bfTypeM;
    int32_t bfSize;
    int16_t bfReserved1;
    int16_t bfReserved2;
    int32_t bfOffBits;
} BITMAPFILEHEADER_struct;

typedef struct {
    uint16_t bcWidth;
    uint16_t bcHeight;
    uint16_t bcPlanes;
    uint16_t bcBitCount;
} BITMAPCOREHEADER_struct;

typedef struct {
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER_struct;

typedef struct {
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
    uint32_t bV4RedMask;
    uint32_t bV4GreenMask;
    uint32_t bV4BlueMask;
    uint32_t bV4AlphaMask;
    uint32_t bV4CSType;
    uint32_t stub1;
    uint32_t stub2;
    uint32_t stub3;
    uint32_t stub4;
    uint32_t stub5;
    uint32_t stub6;
    uint32_t stub7;
    uint32_t stub8;
    uint32_t stub9;
    uint32_t bV4GammaRed;
    uint32_t bV4GammaGreen;
    uint32_t bV4GammaBlue;
} BITMAPV4HEADER_struct;

typedef struct {
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
    uint32_t bV4RedMask;
    uint32_t bV4GreenMask;
    uint32_t bV4BlueMask;
    uint32_t bV4AlphaMask;
    uint32_t bV4CSType;
    uint32_t stub1;
    uint32_t stub2;
    uint32_t stub3;
    uint32_t stub4;
    uint32_t stub5;
    uint32_t stub6;
    uint32_t stub7;
    uint32_t stub8;
    uint32_t stub9;
    uint32_t bV4GammaRed;
    uint32_t bV4GammaGreen;
    uint32_t bV4GammaBlue;
    uint32_t bV5Intent;
    uint32_t bV5ProfileData;
    uint32_t bV5ProfileSize;
    uint32_t bV5Reserved;
} BITMAPV5HEADER_struct;

#pragma pack(pop)

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t reserved;
} palette_entry_t;

static tsgl_imageInfo _parse(const char* path, tsgl_framebuffer* sprite_fb, tsgl_rawcolor transparentColor) {
    tsgl_imageInfo info = {0};

    FILE* file = tsgl_filesystem_open(path, "rb");
    if (file == NULL) return info;

    // check & read header
    BITMAPFILEHEADER_struct BITMAPFILEHEADER;
    fread(&BITMAPFILEHEADER, 1, sizeof(BITMAPFILEHEADER), file);
    if (BITMAPFILEHEADER.bfTypeB != 'B' || BITMAPFILEHEADER.bfTypeM != 'M') {
        ESP_LOGE(TAG, "BMP ERROR: invalid bmp signature: %c%c\n", BITMAPFILEHEADER.bfTypeB, BITMAPFILEHEADER.bfTypeM);
        fclose(file);
        return info;
    }

    // read info
    uint32_t bcSize;
    fread(&bcSize, sizeof(uint32_t), 1, file);
    
    uint32_t palette_size = 0;
    uint32_t palette_entries = 0;
    uint32_t compression = 0;
    switch (bcSize) {
        case 12 : {
            BITMAPCOREHEADER_struct BITMAPINFO;
            fread(&BITMAPINFO, 1, sizeof(BITMAPINFO), file);
            info.width = BITMAPINFO.bcWidth;
            info.height = BITMAPINFO.bcHeight;
            info.bits = BITMAPINFO.bcBitCount;
            
            palette_size = 0;  // для 12-байтового заголовка палитра состоит из 3-байтовых записей
            break;
        }

        case 40 : {
            BITMAPINFOHEADER_struct BITMAPINFO;
            fread(&BITMAPINFO, 1, sizeof(BITMAPINFO), file);
            info.width = BITMAPINFO.biWidth;
            info.height = BITMAPINFO.biHeight;
            info.bits = BITMAPINFO.biBitCount;

            compression = BITMAPINFO.biCompression;
            palette_entries = BITMAPINFO.biClrUsed;
            if (palette_entries == 0 && info.bits <= 8) {
                palette_entries = 1 << info.bits; // если 0, то полная палитра
            }
            palette_size = 4;
            break;
        }

        case 108 : {
            BITMAPV4HEADER_struct BITMAPINFO;
            fread(&BITMAPINFO, 1, sizeof(BITMAPINFO), file);
            info.width = BITMAPINFO.biWidth;
            info.height = BITMAPINFO.biHeight;
            info.bits = BITMAPINFO.biBitCount;

            compression = BITMAPINFO.biCompression;
            palette_entries = BITMAPINFO.biClrUsed;
            if (palette_entries == 0 && info.bits <= 8) palette_entries = 1 << info.bits;
            palette_size = 4;
            break;
        }

        case 124 : {
            BITMAPV5HEADER_struct BITMAPINFO;
            fread(&BITMAPINFO, 1, sizeof(BITMAPINFO), file);
            info.width = BITMAPINFO.biWidth;
            info.height = BITMAPINFO.biHeight;
            info.bits = BITMAPINFO.biBitCount;

            compression = BITMAPINFO.biCompression;
            palette_entries = BITMAPINFO.biClrUsed;
            if (palette_entries == 0 && info.bits <= 8) palette_entries = 1 << info.bits;
            palette_size = 4;
            break;
        }

        default : {
            ESP_LOGE(TAG, "BMP ERROR: unsupported BITMAPINFO: %li\n", bcSize);
            fclose(file);
            return info;
        }
    }

    palette_entry_t palette[256];
    memset(palette, 0, sizeof(palette));

    // Если битность <= 8, читаем палитру
    if (info.bits <= 8) {
        // Определяем размер одной записи в палитре
        size_t entry_size = (bcSize == 12) ? 3 : 4; // для старого формата – 3 байта (RGB), иначе 4 (BGRA)
        for (uint32_t i = 0; i < palette_entries; i++) {
            if (entry_size == 4) {
                uint8_t b, g, r, reserved;
                fread(&b, 1, 1, file);
                fread(&g, 1, 1, file);
                fread(&r, 1, 1, file);
                fread(&reserved, 1, 1, file);
                palette[i].blue  = b;
                palette[i].green = g;
                palette[i].red   = r;
                palette[i].reserved = reserved;
            } else { // 3 байта
                uint8_t r, g, b;
                fread(&r, 1, 1, file);
                fread(&g, 1, 1, file);
                fread(&b, 1, 1, file);
                palette[i].red   = r;
                palette[i].green = g;
                palette[i].blue  = b;
                palette[i].reserved = 0;
            }
        }
    }

    info.reverseLines = info.height > 0;
    info.height = abs(info.height);

    if (sprite_fb) {
        if (info.bits != 8 && info.bits != 24 && info.bits != 32) {
            ESP_LOGE(TAG, "BMP ERROR: unsupported bit depth %d", info.bits);
        }

        fseek(file, BITMAPFILEHEADER.bfOffBits, SEEK_SET);

        uint32_t rowSize = ((info.bits * info.width + 31) / 32) * 4;

        uint8_t* bmpBuffer = malloc(BMP_BUFFER_SIZE);
        size_t bmpBufferPos = BMP_BUFFER_SIZE;

        uint8_t bmpRead() {
            if (bmpBufferPos >= BMP_BUFFER_SIZE) {
                fread(bmpBuffer, 1, BMP_BUFFER_SIZE, file);
                bmpBufferPos = 0;
            }
            return bmpBuffer[bmpBufferPos++];
        }

        for (tsgl_pos iy = 0; iy < info.height; iy++) {
            for (tsgl_pos ix = 0; ix < info.width; ix++) {
                uint8_t red = 0, green = 0, blue = 0, alpha = 255;

                if (info.bits == 8) {
                    uint8_t idx = bmpRead();
                    if (idx < palette_entries) {
                        red   = palette[idx].red;
                        green = palette[idx].green;
                        blue  = palette[idx].blue;
                    } else {
                        ESP_LOGW(TAG, "BMP: palette index %d out of range", idx);
                    }
                } else if (info.bits == 24) {
                    blue  = bmpRead();
                    green = bmpRead();
                    red   = bmpRead();
                } else if (info.bits == 32) {
                    blue  = bmpRead();
                    green = bmpRead();
                    red   = bmpRead();
                    alpha = bmpRead();
                }

                tsgl_pos iiy = info.reverseLines ? (info.height - iy - 1) : iy;
                if (alpha > 0) {
                    tsgl_framebuffer_set(sprite_fb, ix, iiy, tsgl_color_raw(tsgl_color_pack(red, green, blue), sprite_fb->colormode));
                } else {
                    tsgl_framebuffer_set(sprite_fb, ix, iiy, transparentColor);
                }
            }
        }

        free(bmpBuffer);
    }

    fclose(file);
    return info;
}

tsgl_imageInfo tsgl_bmp_readImageInfo(const char* path) {
    return _parse(path, NULL, TSGL_INVALID_RAWCOLOR);
}

tsgl_sprite* tsgl_bmp_load(const char* path, tsgl_colormode colormode, int64_t caps, tsgl_rawcolor transparentColor) {
    tsgl_sprite* sprite = calloc(1, sizeof(tsgl_sprite));
    tsgl_framebuffer* sprite_fb = malloc(sizeof(tsgl_framebuffer));
    sprite->sprite = sprite_fb;
    sprite->transparentColor = transparentColor;

    tsgl_imageInfo imageInfo = _parse(path, NULL, TSGL_INVALID_RAWCOLOR);
    if (imageInfo.width == 0) {
        ESP_LOGW(TAG, "failed to read bmp info: %s", path);
        free(sprite);
        free(sprite_fb);
        return NULL;
    }

    if (tsgl_framebuffer_init(sprite_fb, colormode, imageInfo.width, imageInfo.height, caps) != ESP_OK) {
        ESP_LOGW(TAG, "failed to allocate bmp framebuffer: %s", path);
        free(sprite);
        free(sprite_fb);
        return NULL;
    }

    imageInfo = _parse(path, sprite_fb, transparentColor);
    if (imageInfo.width == 0) {
        ESP_LOGW(TAG, "failed to parse bmp: %s", path);
        free(sprite);
        free(sprite_fb);
        return NULL;
    }

    ESP_LOGI(TAG, "bmp loaded: %s", path);
    return sprite;
}

void tsgl_bmp_free(tsgl_sprite* sprite) {
    tsgl_framebuffer_free(sprite->sprite);
    free(sprite->sprite);
    free(sprite);
}