#include "bmp.h"
#include "display.h"

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

static ImageInfo _parse(const char* path, uint16_t** bufferPtr) {
    ImageInfo info = {0};

    FILE* file = fopen(path, "rb");
    if (file == NULL) return info;

    // check & read header
    BITMAPFILEHEADER_struct BITMAPFILEHEADER;
    fread(&BITMAPFILEHEADER, 1, sizeof(BITMAPFILEHEADER), file);
    if (BITMAPFILEHEADER.bfTypeB != 'B' || BITMAPFILEHEADER.bfTypeM != 'M') {
        printf("BMP ERROR: invalid bmp signature: %c%c\n", BITMAPFILEHEADER.bfTypeB, BITMAPFILEHEADER.bfTypeM);
        fclose(file);
        return info;
    }

    // read info
    uint32_t bcSize;
    fread(&bcSize, sizeof(uint32_t), 1, file);
    switch (bcSize) {
        case 12 : {
            BITMAPCOREHEADER_struct BITMAPINFO;
            fread(&BITMAPINFO, 1, sizeof(BITMAPINFO), file);
            info.width = BITMAPINFO.bcWidth;
            info.height = BITMAPINFO.bcHeight;
            info.bits = BITMAPINFO.bcBitCount;
            break;
        }

        case 40 : {
            BITMAPINFOHEADER_struct BITMAPINFO;
            fread(&BITMAPINFO, 1, sizeof(BITMAPINFO), file);
            info.width = BITMAPINFO.biWidth;
            info.height = BITMAPINFO.biHeight;
            info.bits = BITMAPINFO.biBitCount;
            break;
        }

        case 108 : {
            BITMAPV4HEADER_struct BITMAPINFO;
            fread(&BITMAPINFO, 1, sizeof(BITMAPINFO), file);
            info.width = BITMAPINFO.biWidth;
            info.height = BITMAPINFO.biHeight;
            info.bits = BITMAPINFO.biBitCount;
            break;
        }

        case 124 : {
            BITMAPV5HEADER_struct BITMAPINFO;
            fread(&BITMAPINFO, 1, sizeof(BITMAPINFO), file);
            info.width = BITMAPINFO.biWidth;
            info.height = BITMAPINFO.biHeight;
            info.bits = BITMAPINFO.biBitCount;
            break;
        }

        default : {
            printf("BMP ERROR: unsupported BITMAPINFO: %li\n", bcSize);
            fclose(file);
            return info;
        }
    }

    info.reverseLines = info.height > 0;
    info.height = abs(info.height);

    if (bufferPtr) {
        uint16_t* imageBuffer = malloc(info.width * info.height * sizeof(uint16_t));

        fseek(file, BITMAPFILEHEADER.bfOffBits, SEEK_SET);

        uint8_t* bmpBuffer = malloc(BMP_BUFFER_SIZE);
        size_t bmpBufferPos = BMP_BUFFER_SIZE;

        uint8_t bmpRead() {
            if (bmpBufferPos >= BMP_BUFFER_SIZE) {
                fread(bmpBuffer, 1, BMP_BUFFER_SIZE, file);
                bmpBufferPos = 0;
            }
            return bmpBuffer[bmpBufferPos++];
        }

        size_t bufferPointer = 0;
        for (int iy = 0; iy < info.height; iy++) {
            for (int ix = 0; ix < info.width; ix++) {
                uint8_t blue = bmpRead();
                uint8_t green = bmpRead();
                uint8_t red = bmpRead();
                uint8_t alpha = 255;
                if (info.bits == 32) {
                    alpha = bmpRead();
                }

                uint32_t color = 0;
                if (alpha > 0) {
                    color = (red << 16) | (green << 8) | blue;
                }

                //imageBuffer[bufferPointer++] = rgb888_to_rgb565(color);
            }
        }

        *bufferPtr = imageBuffer;
        free(bmpBuffer);
    }

    fclose(file);
    return info;
}

ImageInfo bmp_readImageInfo(const char* path) {
    return _parse(path, NULL);
}

Image* bmp_load(const char* path) {
    Image* image = malloc(sizeof(Image));
    ImageInfo imageInfo = _parse(path, &image->img); //pointer to pointer!
    if (imageInfo.width == 0) {
        free(image);
        return NULL;
    }
    image->width = imageInfo.width;
    image->height = imageInfo.height;
    return image;
}