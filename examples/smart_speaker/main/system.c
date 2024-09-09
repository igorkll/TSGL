// -------------------------------- display settings

#include "system.h"
#include <TSGL_drivers/st77XX.h>

#define SPI    TSGL_HOST1
#define FREQ   80000000
#define BUFFER TSGL_SPIRAM
#define DC     21
#define CS     22
#define RST    33
#define BL     4

#define WIDTH 320
#define HEIGHT 480
#define ROTATION 1

static tsgl_display_settings settings = {
    .driver = &st77XX_rgb565,
    .swapRGB = true,
    .invert = true,
    .flipX = true,
    .width = WIDTH,
    .height = HEIGHT
};

// -------------------------------- touchscreen settings

#define TS_SDA   5
#define TS_SCL   27
#define TS_HOST  I2C_NUM_0
#define TS_ADDR  0x38
#define TS_RST   23

// -------------------------------- other

#define POWERLOCK 32

#define RED  18
#define BLUE 19

#define FIRST 34
#define LAST  35

// --------------------------------

tsgl_display display;
tsgl_framebuffer framebuffer;
tsgl_framebuffer framebuffer2;
tsgl_benchmark benchmark;
tsgl_touchscreen touchscreen = {
    .width = WIDTH,
    .height = HEIGHT
};
tsgl_keyboard keyboard;

static tsgl_ledc led_red;
static tsgl_ledc led_blue;

void system_init() {
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer, settings.driver->colormode, settings.width, settings.height, BUFFER));

    tsgl_print_settings print = {
        .font = tsgl_font_defaultFont,
        .fill = TSGL_INVALID_RAWCOLOR,
        .bg = TSGL_INVALID_RAWCOLOR,
        .fg = tsgl_color_raw(tsgl_color_fromHex(0xda02c1), settings.driver->colormode),
        .multiline = true,
        .globalCentering = true,
        .locationMode = tsgl_print_start_top
    };
    tsgl_framebuffer_rotate(&framebuffer, ROTATION);
    print.width = framebuffer.width;
    print.height = framebuffer.height;
    print.targetHeight = framebuffer.width / 6;
    tsgl_framebuffer_clear(&framebuffer, tsgl_color_raw(tsgl_color_fromHex(0x091078), settings.driver->colormode));
    tsgl_framebuffer_text(&framebuffer, 0, 0, print, "SMART\nSPEAKER");
    tsgl_framebuffer_rotate(&framebuffer, 0);

    settings.init_state = tsgl_display_init_framebuffer;
    settings.init_framebuffer_ptr = framebuffer.buffer;
    settings.init_framebuffer_size = framebuffer.buffersize;

    settings.backlight_init = true;
    settings.backlight_pin = BL;
    settings.backlight_value = 0;

    ESP_ERROR_CHECK(tsgl_spi_init(tsgl_math_maxSendSize(settings), SPI));
    ESP_ERROR_CHECK(tsgl_display_spi(&display, settings, SPI, FREQ, DC, CS, RST));
    tsgl_display_incompleteSending(&display, false, NULL);
    tsgl_display_setBacklight(&display, 255);
    ESP_ERROR_CHECK(tsgl_i2c_init(TS_HOST, TS_SDA, TS_SCL));
    ESP_ERROR_CHECK(tsgl_touchscreen_ft6336u(&touchscreen, TS_HOST, TS_ADDR, TS_RST));
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer2, display.colormode, settings.width, settings.height, BUFFER));
    ESP_ERROR_CHECK(tsgl_filesystem_mount_fatfs("/storage", "storage"));

    tsgl_framebuffer_hardwareRotate(&framebuffer, ROTATION);
    tsgl_display_rotate(&display, ROTATION);
    touchscreen.rotation = ROTATION;

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask |= 1ULL << POWERLOCK;
    io_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_conf);
    gpio_set_level(POWERLOCK, false);

    tsgl_ledc_new(&led_red, RED, false, 0);
    tsgl_ledc_new(&led_blue, BLUE, false, 0);

    tsgl_keyboard_init(&keyboard);
    tsgl_keyboard_bindButton(&keyboard, 'A', false, false, FIRST);
    tsgl_keyboard_bindButton(&keyboard, 'B', false, false, LAST);
    tsgl_keyboard_setDebounce(&keyboard, 'A', 20, 50);
    tsgl_keyboard_setDebounce(&keyboard, 'B', 20, 50);
}

void system_powerOff() {
    tsgl_display_setBacklight(&display, 0);
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask |= 1ULL << POWERLOCK;
    io_conf.mode = GPIO_MODE_DISABLE;
    gpio_config(&io_conf);
    while (true) vTaskDelay(1);
}

void system_setRed(uint8_t value) {
    tsgl_ledc_set(&led_red, value);
}

void system_setBlue(uint8_t value) {
    tsgl_ledc_set(&led_blue, value);
}