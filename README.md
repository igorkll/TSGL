# gfxTSGL
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
* all color constants have the tsgl_color data type and require conversion to be sent to the screen or written to the framebuffer
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

# TSGL_framebuffer.h
```c
typedef struct {
    uint8_t* buffer;
    size_t buffersize;
    tsgl_pos width; //the width and height fields are swapped when calling tsgl_framebuffer_rotate with parameter 1 or 3
    tsgl_pos height;
    tsgl_pos defaultWidth;
    tsgl_pos defaultHeight;
    uint8_t colorsize;
    uint8_t rotation; //read-only! to change the rotation, use a special method
    tsgl_colormode colormode;
    tsgl_rawcolor black; //the black color that is used inside the framebuffer, however, you can use it for the same framebuffer since it is converted specifically for it
} tsgl_framebuffer;

esp_err_t tsgl_framebuffer_init(tsgl_framebuffer* framebuffer, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height, int64_t caps);
void tsgl_framebuffer_free(tsgl_framebuffer* framebuffer);
void tsgl_framebuffer_rotate(tsgl_framebuffer* framebuffer, uint8_t rotation); //rotates the indexing of the framebuffer and not the framebuffer itself
tsgl_rawcolor tsgl_framebuffer_get(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y);

void tsgl_framebuffer_set(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color);
void tsgl_framebuffer_fill(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color);
void tsgl_framebuffer_rect(tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color);
void tsgl_framebuffer_clear(tsgl_framebuffer* framebuffer, tsgl_rawcolor color);
```
## framebuffer example
```c
#include <TSGL.h>
#include <TSGL_framebuffer.h>
#include <TSGL_display.h>
#include <TSGL_color.h>
#include <TSGL_spi.h>

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
#include <esp_err.h>

#define WIDTH       320
#define HEIGHT      240
#define COLORMODE   tsgl_rgb565_be
#define FREQUENCY   60000000
#define DISPLAY_SPI TSGL_HOST1
#define DISPLAY_DC  21
#define DISPLAY_CS  22
#define DISPLAY_RST 18
//#define CUSTOM_SPI_GPIO //when using custom GPIO pins instead of standard ones, the frequency cannot be higher than 26 megahertz (20000000 recommended)
//#define CUSTOM_MISO x
//#define CUSTOM_MOSI x
//#define CUSTOM_CLK x

tsgl_framebuffer framebuffer;
tsgl_display display;

void app_main() {
    #ifdef CUSTOM_SPI_GPIO
        ESP_ERROR_CHECK(tsgl_spi_initManual(WIDTH * HEIGHT * tsgl_colormodeSizes[COLORMODE], TSGL_HOST1, CUSTOM_MISO, CUSTOM_MOSI, CUSTOM_CLK));
    #else
        ESP_ERROR_CHECK(tsgl_spi_init(WIDTH * HEIGHT * tsgl_colormodeSizes[COLORMODE], TSGL_HOST1));
    #endif
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer, COLORMODE, WIDTH, HEIGHT, 0));
    ESP_ERROR_CHECK(tsgl_display_spi(&display, WIDTH, HEIGHT, TSGL_HOST1, FREQUENCY, DISPLAY_DC, DISPLAY_CS, DISPLAY_RST));
    
    tsgl_framebuffer_rotate(&framebuffer, 3); //making the screen vertical
    tsgl_pos margin = 32;
    uint8_t hue = 0;
    while (true) {
        tsgl_framebuffer_clear(&framebuffer, framebuffer.black);
        tsgl_framebuffer_fill(&framebuffer, margin, margin, framebuffer.width - (margin * 2), framebuffer.height - (margin * 2), tsgl_color_raw(tsgl_color_hsv(hue++, 255, 255), COLORMODE));
        tsgl_display_send(&display, &framebuffer);
    }

    tsgl_framebuffer_free(&framebuffer); //if you don't need this framebuffer anymore, then you should unload it.
    tsgl_display_free(&display);
}
```