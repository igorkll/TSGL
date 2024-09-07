#include "TSGL.h"
#include "TSGL_color.h"
#include "TSGL_math.h"
#include <string.h>

const tsgl_color TSGL_WHITE = {
    .r = 255,
    .g = 255,
    .b = 255
};

const tsgl_color TSGL_ORANGE = {
    .r = 255,
    .g = 180,
    .b = 0
};

const tsgl_color TSGL_MAGENTA = {
    .r = 255,
    .g = 0,
    .b = 255
};

const tsgl_color TSGL_YELLOW = {
    .r = 255,
    .g = 255,
    .b = 0
};

const tsgl_color TSGL_GRAY = {
    .r = 127,
    .g = 127,
    .b = 127
};

const tsgl_color TSGL_CYAN = {
    .r = 0,
    .g = 255,
    .b = 255
};

const tsgl_color TSGL_BLUE = {
    .r = 0,
    .g = 0,
    .b = 255
};

const tsgl_color TSGL_GREEN = {
    .r = 0,
    .g = 255,
    .b = 0
};

const tsgl_color TSGL_RED = {
    .r = 255,
    .g = 0,
    .b = 0
};

const tsgl_color TSGL_BLACK = {
    .r = 0,
    .g = 0,
    .b = 0
};

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

tsgl_color tsgl_color_mul(tsgl_color color, float mul) {
    tsgl_color result = {
        .r = TSGL_MATH_CLAMP(color.r * mul, 0, 255),
        .g = TSGL_MATH_CLAMP(color.g * mul, 0, 255),
        .b = TSGL_MATH_CLAMP(color.b * mul, 0, 255)
    };
    return result;
}

tsgl_color tsgl_color_hsv(uint8_t hue, uint8_t saturation, uint8_t value) {
    tsgl_color rgb;
    uint8_t region, remainder, p, q, t;

    if (saturation == 0) {
        rgb.r = value;
        rgb.g = value;
        rgb.b = value;
        return rgb;
    }

    region = hue / 43;
    remainder = (hue - (region * 43)) * 6;

    p = (value * (255 - saturation)) >> 8;
    q = (value * (255 - ((saturation * remainder) >> 8))) >> 8;
    t = (value * (255 - ((saturation * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            rgb.r = value; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = value; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = value; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = value;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = value;
            break;
        default:
            rgb.r = value; rgb.g = p; rgb.b = q;
            break;
    }

    return rgb;
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
    if (color.invalid) return TSGL_INVALID_RAWCOLOR;
    tsgl_rawcolor rawcolor = {0};
    uint16_t* access16 = (uint16_t*)(rawcolor.arr);
    switch (colormode) {
        case tsgl_rgb565_le : {
            *access16 = ((color.r >> 3) << 11) | ((color.g >> 2) << 5) | (color.b >> 3);
            break;
        }

        case tsgl_bgr565_le : {
            *access16 = ((color.b >> 3) << 11) | ((color.g >> 2) << 5) | (color.r >> 3);
            break;
        }

        case tsgl_rgb565_be : {
            *access16 = ((color.r >> 3) << 3) | (color.g >> 5) | ((color.g >> 2) << 13) | ((color.b >> 3) << 8);
            break;
        }

        case tsgl_bgr565_be : {
            *access16 = ((color.b >> 3) << 3) | (color.g >> 5) | ((color.g >> 2) << 13) | ((color.r >> 3) << 8);
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
            uint8_t r = color.r >> 4;
            uint8_t g = color.g >> 4;
            uint8_t b = color.b >> 4;
            rawcolor.arr[0] = (r << 4) | g;
            rawcolor.arr[1] = (b << 4) | r;
            rawcolor.arr[2] = (g << 4) | b;
            break;
        }

        case tsgl_bgr444 : {
            uint8_t r = color.r >> 4;
            uint8_t g = color.g >> 4;
            uint8_t b = color.b >> 4;
            rawcolor.arr[0] = (b << 4) | g;
            rawcolor.arr[1] = (r << 4) | b;
            rawcolor.arr[2] = (g << 4) | r;
            break;
        }

        case tsgl_monochrome:
            rawcolor.arr[0] = color.r > 0 || color.g > 0 || color.b > 0;
            break;
    }
    return rawcolor;
}

tsgl_color tsgl_color_uraw(tsgl_rawcolor rawcolor, tsgl_colormode colormode) {
    if (rawcolor.invalid) return TSGL_INVALID_COLOR;
    switch (colormode) {
        case tsgl_rgb565_le : {
            return tsgl_color_from565(rawcolor.arr[0] + (rawcolor.arr[1] << 8));
        }

        case tsgl_bgr565_le : {
            tsgl_color color = tsgl_color_from565(rawcolor.arr[0] + (rawcolor.arr[1] << 8));
            uint8_t t = color.b;
            color.b = color.r;
            color.r = t;
            return color;
        }

        case tsgl_rgb565_be : {
            return tsgl_color_from565((rawcolor.arr[0] << 8) + rawcolor.arr[1]);
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
                .r = rawcolor.arr[0] & 0b11110000,
                .g = (rawcolor.arr[0] & 0b1111) << 4,
                .b = rawcolor.arr[1] & 0b11110000
            };
            return color;
        }

        case tsgl_bgr444 : {
            tsgl_color color = {
                .b = rawcolor.arr[0] & 0b11110000,
                .g = (rawcolor.arr[0] & 0b1111) << 4,
                .r = rawcolor.arr[1] & 0b11110000
            };
            return color;
        }

        case tsgl_monochrome:
            uint8_t val = rawcolor.arr[0] ? 255 : 0;
            tsgl_color color = {
                .r = val,
                .g = val,
                .b = val
            };
            return color;
    }
    return TSGL_BLACK;
}

bool tsgl_color_rawColorCompare(tsgl_rawcolor color1, tsgl_rawcolor color2, float colorsize) {
    if (colorsize == (int)colorsize) {
        return memcmp(color1.arr, color2.arr, colorsize) == 0;
    } else {
        uint8_t byteIndex = 0;
        uint8_t bitsCount = colorsize * 8;
        while (bitsCount >= 8) {
            if (color1.arr[byteIndex] != color2.arr[byteIndex]) {
                return false;
            }
            bitsCount -= 8;
            byteIndex++;
        }
        for (size_t i = 0; i < bitsCount; i++) {
            if (((color1.arr[byteIndex] >> i) & 1) != ((color2.arr[byteIndex] >> i) & 1)) {
                return false;
            }
        }
        return true;
    }
}