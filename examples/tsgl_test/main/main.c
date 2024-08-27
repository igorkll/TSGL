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

void* buttonCallback(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0) {
        self->root->color = tsgl_color_raw(tsgl_color_fromHex(0x019cb4), self->colormode);
    } else {
        self->root->color = tsgl_color_raw(tsgl_color_fromHex(0x018db4), self->colormode);
    }
    self->root->needDraw = true;
    return NULL;
}

void test_gui() {
    tsgl_gui* gui = tsgl_gui_createRoot_buffer(&display, &framebuffer);
    gui->color = tsgl_color_raw(tsgl_color_fromHex(0x018db4), gui->colormode);

    tsgl_gui* button = tsgl_gui_addButton(gui);
    tsgl_gui_setAllFormat(button, tsgl_gui_percentMaxSide);
    button->x = 0.1;
    button->y = 0.1;
    button->width = 0.4;
    button->height = 0.4;
    button->user_callback = buttonCallback;
    tsgl_gui* buttonText = tsgl_gui_button_getTextChild(button);
    tsgl_gui_text_setText(buttonText, "TEST", false);

    while (true) {
        tsgl_gui_processTouchscreen(gui, &touchscreen);
        tsgl_gui_processGui(gui, &framebuffer2, &benchmark);
        tsgl_benchmark_print(&benchmark);
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