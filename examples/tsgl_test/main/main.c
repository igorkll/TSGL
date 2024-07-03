// -------------------------------- display settings
#include <TSGL_drivers/st77XX.h>

#define SPI  TSGL_HOST1
#define FREQ 60000000
#define BUFFER TSGL_SPIRAM
#define DC 21
#define CS 22
#define RST 18
#define BL 5

const tsgl_settings settings = {
    .driver = &st77XX_rgb565,
    .swapRGB = true,
    .invert = true,
    .flipX = true,
    .width = 320,
    .height = 480
};

// -------------------------------- touchscreen settings

#include <TSGL_i2c.h>

#define TS_SDA 26
#define TS_SCL 27
#define TS_HOST I2C_NUM_0
#define TS_ADDR 0x38
#define TS_RST 25

// --------------------------------
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <math.h>

#include <TSGL.h>
#include <TSGL_benchmark.h>
#include <TSGL_framebuffer.h>
#include <TSGL_touchscreen.h>
#include <TSGL_display.h>
#include <TSGL_color.h>
#include <TSGL_font.h>
#include <TSGL_spi.h>
#include <TSGL_gui.h>

#include <TSGL_fonts/font.h>
#include <TSGL_gui/button.h>
#include <TSGL_gui/sprite.h>

tsgl_display display;
tsgl_framebuffer framebuffer;
tsgl_framebuffer framebuffer2;
tsgl_benchmark benchmark;
tsgl_touchscreen touchscreen = {
    .width = settings.width,
    .height = settings.height
};

float fmap(float value, float low, float high, float low_2, float high_2) {
    float relative_value = (value - low) / (high - low);
    float scaled_value = low_2 + (high_2 - low_2) * relative_value;
    return scaled_value;
}

int imap(int value, int low, int high, int low_2, int high_2) {
    return (int)(fmap(value, low, high, low_2, high_2) + 0.5);
}

void delay(int time) {
    vTaskDelay(time / portTICK_PERIOD_MS);
}

void test_gui() {
    tsgl_framebuffer* sprite = tsgl_framebuffer_new(display.colormode, 300, 150, BUFFER);
    tsgl_framebuffer_clear(sprite, sprite->black);
    for (int i = 0; i < 150; i++) {
        tsgl_framebuffer_set(sprite, esp_random() % sprite->width, esp_random() % sprite->height, tsgl_color_raw(TSGL_RED, sprite->colormode));
        tsgl_framebuffer_set(sprite, esp_random() % sprite->width, esp_random() % sprite->height, tsgl_color_raw(TSGL_GREEN, sprite->colormode));
    }

    tsgl_print_settings sets = {
        .font = font,
        .bg = TSGL_INVALID_RAWCOLOR,
        .fg = tsgl_color_raw(TSGL_ORANGE, sprite->colormode),
        .locationMode = tsgl_print_start_top,
        .scale = 0.6
    };
    tsgl_print_textArea textArea = tsgl_framebuffer_text(sprite, 1, 1, sets, "LOLZ");
    tsgl_framebuffer_rect(sprite, textArea.left, textArea.top, textArea.width, textArea.height, tsgl_color_raw(TSGL_BLUE, sprite->colormode), 3);

    tsgl_gui* gui = tsgl_gui_createRoot_buffer(&display, &framebuffer);
    //tsgl_gui* gui = tsgl_gui_createRoot_display(&display, display.colormode);
    gui->color = tsgl_color_raw(tsgl_color_fromHex(0x3b3b3b), gui->colormode);

    tsgl_gui* button4 = tsgl_gui_addButton(gui);
    button4->x = 50;
    button4->y = 50;
    button4->width = 100;
    button4->height = 100;
    tsgl_gui_text_setParams(button4->children[0], tsgl_font_defaultFont, 0.4, TSGL_INVALID_COLOR, TSGL_WHITE);
    tsgl_gui_text_setText(button4->children[0], "TEST\nQWER\n123456789", false);

    /*
    tsgl_sprite spriteData = {
        .rotation = 0,
        .sprite = sprite,
        .transparentColor = TSGL_INVALID_RAWCOLOR
    };

    tsgl_gui* window = tsgl_gui_addSprite(gui, &spriteData);
    window->x = 50;
    window->y = 50;
    window->width = 300;
    window->height = 150;
    window->draggable = true;

    tsgl_gui* window2 = tsgl_gui_addObject(gui);
    window2->color = tsgl_color_raw(TSGL_YELLOW, gui->colormode);
    window2->x = 200;
    window2->y = 200;
    window2->width = 100;
    window2->height = 100;
    window2->draggable = true;

    tsgl_gui* window3 = tsgl_gui_addObject(gui);
    window3->color = tsgl_color_raw(TSGL_GREEN, gui->colormode);
    window3->x = 200;
    window3->y = 200;
    window3->width = 100;
    window3->height = 100;
    window3->draggable = true;

    tsgl_gui* testPlane = tsgl_gui_addObject(window);
    testPlane->color = tsgl_color_raw(tsgl_color_fromHex(0xbdbdbd), gui->colormode);
    testPlane->x = 250;
    testPlane->y = 0;
    testPlane->width = 50;
    testPlane->height = 150;

    tsgl_gui* button = tsgl_gui_addButton(testPlane);
    button->x = 0;
    button->y = 0;
    button->width = 50;
    button->height = 50;
    button->draggable = true;

    tsgl_gui* button2 = tsgl_gui_addButton(window);
    button2->x = 10;
    button2->y = 10;
    button2->width = 50;
    button2->height = 50;
    button2->draggable = true;

    tsgl_gui* button3 = tsgl_gui_addObject(button);
    button3->color = tsgl_color_raw(tsgl_color_fromHex(0xffffff), gui->colormode);
    button3->x = 10;
    button3->y = 10;
    button3->width = 30;
    button3->height = 40;
    button3->draggable = true;
    */

    while (true) {
        tsgl_gui_processTouchscreen(gui, &touchscreen);
        tsgl_gui_processGui(gui, &framebuffer2, &benchmark);
        //tsgl_benchmark_print(&benchmark);
    }

    tsgl_gui_free(gui);
}

void app_main() {
    tsgl_display_pushBacklight(BL, 0);
    ESP_ERROR_CHECK(tsgl_spi_init(settings.width * settings.height * tsgl_colormodeSizes[settings.driver->colormode], SPI));
    tsgl_display_pushInitColor(tsgl_color_raw(TSGL_RED, settings.driver->colormode));
    ESP_ERROR_CHECK(tsgl_display_spi(&display, settings, SPI, FREQ, DC, CS, RST));
    tsgl_display_setBacklight(&display, 255);
    ESP_ERROR_CHECK(tsgl_i2c_init(TS_HOST, TS_SDA, TS_SCL));
    ESP_ERROR_CHECK(tsgl_touchscreen_ft6336u(&touchscreen, TS_HOST, TS_ADDR, TS_RST));
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer, display.colormode, settings.width, settings.height, BUFFER));
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer2, display.colormode, settings.width, settings.height, BUFFER));

    tsgl_framebuffer_hardwareRotate(&framebuffer, 1);
    tsgl_display_rotate(&display, 1);
    touchscreen.rotation = 1;

    while (true) {
        test_gui();
    }
}