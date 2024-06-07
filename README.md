# espTSGL
* graphics library for ESP32 controllers.
* the "arduino" and "esp-idf" environments are supported.

# TSGL.h
* tsgl_pos - a data type that stores the position of a point on the screen or the size of an object on the screen
* tsgl_colormodeSizes[] - an array that stores the number of bytes for each color mode
```c
enum {
    tsgl_rgb565_le,
    tsgl_rgb565_be,
    tsgl_bgr565_le,
    tsgl_bgr565_be,
    tsgl_rgb888,
    tsgl_bgr888,
} tsgl_colormode;
```

# TSGL_color.h
* TSGL_WHITE - 0xffffff
* TSGL_ORANGE - 0xF2B233
* TSGL_MAGENTA - 0xE57FD8
* TSGL_LIGHT_BLUE - 0x99B2F2
* TSGL_YELLOW - 0xDEDE6C
* TSGL_LIME - 0x7FCC19
* TSGL_PINK - 0xF2B2CC
* TSGL_GRAY - 0x4C4C4C
* TSGL_LIGHT_GRAY - 0x999999
* TSGL_CYAN - 0x4C99B2
* TSGL_PURPLE - 0xB266E5
* TSGL_BLUE - 0x3366CC
* TSGL_BROWN - 0x7F664C
* TSGL_GREEN - 0x57A64E
* TSGL_RED - 0xCC4C4C
* TSGL_BLACK - 0x191919
* tsgl_rawcolor - the data type containing the color after conversion
* tsgl_color - the data type containing the RGB color
```c
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} tsgl_color;

tsgl_color tsgl_color_pack(uint8_t r, uint8_t g, uint8_t b);
tsgl_color tsgl_color_combine(float v, tsgl_color color1, tsgl_color color2);
tsgl_color tsgl_color_hsv(uint8_t hue, uint8_t saturation, uint8_t value);

uint16_t tsgl_color_to565(tsgl_color color);
tsgl_color tsgl_color_from565(uint16_t color);

uint32_t tsgl_color_toHex(tsgl_color color);
tsgl_color tsgl_color_fromHex(uint32_t color);

tsgl_rawcolor tsgl_color_raw(tsgl_color color, tsgl_colormode colormode);
tsgl_color tsgl_color_uraw(tsgl_rawcolor color, tsgl_colormode colormode);
```