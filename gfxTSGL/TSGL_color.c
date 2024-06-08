#include "TSGL.h"
#include "TSGL_color.h"

tsgl_color tsgl_color_pack(uint8_t r, uint8_t g, uint8_t b) {
    tsgl_color result = {
        .r = r,
        .g = g,
        .b = b
    };
    return result;
}

tsgl_color tsgl_color_combine(float v, tsgl_color color1, tsgl_color color2) {
    tsgl_color result = {
        .r = color1.r + v * (color2.r - color1.r),
        .g = color1.g + v * (color2.g - color1.g),
        .b = color1.b + v * (color2.b - color1.b)
    };
    return result;
}

tsgl_color tsgl_color_hsv(uint8_t hue, uint8_t saturation, uint8_t value) {
    float h = hue / 255.0;
    float s = saturation / 255.0;
    float v = value / 255.0;

    float r = 0;
    float g = 0;
    float b = 0;

    int i = h * 6;

    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        case 5:
            r = v;
            g = p;
            b = q;
            break;
    }

    return tsgl_color_pack(r * 255, g * 255, b * 255);
}

uint16_t tsgl_color_to565(tsgl_color color) {
    uint16_t result;
    result = (color.r >> 3) << 11;
    result |= (color.g >> 2) << 5;
    result |= (color.b >> 3);
    return result;
}

tsgl_color tsgl_color_from565(uint16_t color) {
    tsgl_color result = {
        .r = (((color >> 11) & 0x1F) * 255 + 15) / 31,
        .g = (((color >> 5)  & 0x3F) * 255 + 31) / 63,
        .b = (((color)       & 0x1F) * 255 + 15) / 31
    };
    return result;
}

uint32_t tsgl_color_toHex(tsgl_color color) {
    return (color.r << 16) | (color.g << 8) | color.b;
}

tsgl_color tsgl_color_fromHex(uint32_t color) {
    tsgl_color result = {
        .r = (color >> 16) % 256,
        .g = (color >> 8) % 256,
        .b = color % 256
    };
    return result;
}

tsgl_rawcolor tsgl_color_raw(tsgl_color color, tsgl_colormode colormode) {
    tsgl_rawcolor rawcolor;
    switch (colormode) {
        case tsgl_rgb565_le : {
            uint16_t color565 = tsgl_color_to565(color);
            rawcolor.arr[0] = color565 % 256;
            rawcolor.arr[1] = color565 >> 8;
            break;
        }

        case tsgl_rgb565_be : {
            uint16_t color565 = tsgl_color_to565(color);
            rawcolor.arr[0] = color565 >> 8;
            rawcolor.arr[1] = color565 % 256;
            break;
        }

        case tsgl_bgr565_le : {
            uint16_t color565 = tsgl_color_to565(tsgl_color_pack(color.b, color.g, color.r));
            rawcolor.arr[0] = color565 % 256;
            rawcolor.arr[1] = color565 >> 8;
            break;
        }

        case tsgl_bgr565_be : {
            uint16_t color565 = tsgl_color_to565(tsgl_color_pack(color.b, color.g, color.r));
            rawcolor.arr[0] = color565 >> 8;
            rawcolor.arr[1] = color565 % 256;
            break;
        }

        case tsgl_rgb888 : {
            rawcolor.arr[0] = color.r;
            rawcolor.arr[1] = color.g;
            rawcolor.arr[2] = color.b;
            break;
        }

        case tsgl_bgr888 : {
            rawcolor.arr[0] = color.b;
            rawcolor.arr[1] = color.g;
            rawcolor.arr[2] = color.r;
            break;
        }

        case tsgl_rgb444 : {
            rawcolor.arr[0] = color.r >> 4;
            rawcolor.arr[1] = color.g >> 4;
            rawcolor.arr[2] = color.b >> 4;
            break;
        }

        case tsgl_bgr444 : {
            rawcolor.arr[0] = color.b >> 4;
            rawcolor.arr[1] = color.g >> 4;
            rawcolor.arr[2] = color.r >> 4;
            break;
        }
    }
    return rawcolor;
}

tsgl_color tsgl_color_uraw(tsgl_rawcolor rawcolor, tsgl_colormode colormode) {
    switch (colormode) {
        case tsgl_rgb565_le : {
            return tsgl_color_from565(rawcolor.arr[0] + (rawcolor.arr[1] << 8));
        }

        case tsgl_rgb565_be : {
            return tsgl_color_from565((rawcolor.arr[0] << 8) + rawcolor.arr[1]);
        }

        case tsgl_bgr565_le : {
            tsgl_color color = tsgl_color_from565(rawcolor.arr[0] + (rawcolor.arr[1] << 8));
            uint8_t t = color.b;
            color.b = color.r;
            color.r = t;
            return color;
        }

        case tsgl_bgr565_be : {
            tsgl_color color = tsgl_color_from565((rawcolor.arr[0] << 8) + rawcolor.arr[1]);
            uint8_t t = color.b;
            color.b = color.r;
            color.r = t;
            return color;
        }

        case tsgl_rgb888 : {
            tsgl_color color = {
                .r = rawcolor.arr[0],
                .g = rawcolor.arr[1],
                .b = rawcolor.arr[2]
            };
            return color;
        }

        case tsgl_bgr888 : {
            tsgl_color color = {
                .r = rawcolor.arr[2],
                .g = rawcolor.arr[1],
                .b = rawcolor.arr[0]
            };
            return color;
        }

        case tsgl_rgb444 : {
            tsgl_color color = {
                .r = rawcolor.arr[0] << 4,
                .g = rawcolor.arr[1] << 4,
                .b = rawcolor.arr[2] << 4
            };
            return color;
        }

        case tsgl_bgr444 : {
            tsgl_color color = {
                .r = rawcolor.arr[2] << 4,
                .g = rawcolor.arr[1] << 4,
                .b = rawcolor.arr[1] << 4
            };
            return color;
        }

        default : {
            return TSGL_BLACK;
        }
    }
}