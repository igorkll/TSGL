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

void drawTextWithRect(tsgl_pos x, tsgl_pos y, const char* text) {
    tsgl_print_settings printSettings = {
        .font = font,
        .bg = TSGL_INVALID_RAWCOLOR,
        .fg = tsgl_color_raw(TSGL_MAGENTA, framebuffer.colormode)
    };
    
    tsgl_print_textArea textArea = tsgl_font_getTextArea(x, y, printSettings, text);
    tsgl_framebuffer_rect(&framebuffer, textArea.left, textArea.top, textArea.width, textArea.height, tsgl_color_raw(TSGL_BLUE, framebuffer.colormode), 1);
    tsgl_framebuffer_text(&framebuffer, x, y, printSettings, text);
}

void gui_test_onDraw(tsgl_gui* root, void* _) {
    tsgl_display_asyncSend(&display, &framebuffer, &framebuffer2);
}

void gui_test() {
    tsgl_gui* gui = tsgl_gui_createRoot_buffer(&framebuffer);
    tsgl_gui_setClearColor(gui, tsgl_color_raw(tsgl_color_fromHex(0x2a76d5), framebuffer.colormode));

    tsgl_gui* button = tsgl_gui_addButton(gui);
    button->x = 50;
    button->y = 50;
    button->width = 150;
    button->height = 150;
    button->draggable = true;

    while (true) {
        tsgl_gui_processTouchscreen(gui, &touchscreen);
        tsgl_gui_processGui(gui, NULL, gui_test_onDraw);
        delay(portTICK_PERIOD_MS);
    }

    tsgl_gui_free(gui);
}

void app_main() {
    ESP_ERROR_CHECK(tsgl_i2c_init(TS_HOST, TS_SDA, TS_SCL));
    ESP_ERROR_CHECK(tsgl_touchscreen_ft6336u(&touchscreen, TS_HOST, TS_ADDR, TS_RST));
    ESP_ERROR_CHECK(tsgl_spi_init(settings.width * settings.height * tsgl_colormodeSizes[settings.driver->colormode], SPI));
    tsgl_display_pushInitColor(tsgl_color_raw(TSGL_RED, settings.driver->colormode));
    ESP_ERROR_CHECK(tsgl_display_spi(&display, settings, SPI, FREQ, DC, CS, RST));
    ESP_ERROR_CHECK_WITHOUT_ABORT(tsgl_display_attachBacklight(&display, BL, 255));
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer, display.colormode, settings.width, settings.height, BUFFER));
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer2, display.colormode, settings.width, settings.height, BUFFER));

    tsgl_framebuffer_hardwareRotate(&framebuffer, 1);
    tsgl_display_rotate(&display, 1);
    touchscreen.rotation = 1;

    tsgl_rawcolor blue = tsgl_color_raw(TSGL_BLUE, framebuffer.colormode);
    tsgl_rawcolor yellow = tsgl_color_raw(TSGL_YELLOW, framebuffer.colormode);
    tsgl_rawcolor red = tsgl_color_raw(TSGL_RED, framebuffer.colormode);
    tsgl_pos center = framebuffer.width / 2;
    tsgl_pos sinSize = framebuffer.width / 4;
    
    while (true) {
        gui_test();

        tsgl_framebuffer_clear(&framebuffer, display.black);
        tsgl_framebuffer_line(&framebuffer, 0, 0, framebuffer.width, 0, tsgl_color_raw(TSGL_RED, framebuffer.colormode), 5);
        tsgl_framebuffer_line(&framebuffer, 0, 0, framebuffer.width, framebuffer.height, tsgl_color_raw(TSGL_GREEN, framebuffer.colormode), 5);
        tsgl_framebuffer_line(&framebuffer, 0, 0, 0, framebuffer.height, tsgl_color_raw(TSGL_BLUE, framebuffer.colormode), 5);
        drawTextWithRect(20, framebuffer.height - 21, " FONT ");
        tsgl_display_asyncSend(&display, &framebuffer, &framebuffer2);
        delay(3000);

        tsgl_benchmark_reset(&benchmark);
        for (tsgl_pos i = 0; i < display.width; i += tsgl_benchmark_processMulInt(&benchmark, 30)) {
            tsgl_benchmark_startRendering(&benchmark);
            tsgl_framebuffer_clear(&framebuffer, display.black);
            float sinValue = 0;
            tsgl_pos oldY = -1;
            for (tsgl_pos pos = 0; pos < display.width; pos++) {
                float lsin = sin(fmap((pos - i) % sinSize, 0, sinSize, 0, M_PI * 2));
                uint16_t y = (display.height / 2) - (lsin * display.height * 0.4);
                if (pos == center) sinValue = lsin;
                if (oldY >= 0) {
                    tsgl_framebuffer_line(&framebuffer, pos, y, pos, oldY, yellow, 1);
                }
                tsgl_framebuffer_set(&framebuffer, pos, y, yellow);
                oldY = y;
            }
            tsgl_framebuffer_line(&framebuffer, i, 0, i, framebuffer.height - 1, blue, 1);
            tsgl_framebuffer_line(&framebuffer, center, 0, center, framebuffer.height - 1, red, 1);
            uint8_t touchCount = tsgl_touchscreen_touchCount(&touchscreen);
            for (uint8_t i = 0; i < touchCount; i++) {
                tsgl_touchscreen_point point = tsgl_touchscreen_getPoint(&touchscreen, i);
                tsgl_framebuffer_fill(&framebuffer, point.x - 32, point.y - 32, 64, 64, tsgl_color_raw(TSGL_RED, framebuffer.colormode));
            }
            tsgl_benchmark_endRendering(&benchmark);

            tsgl_benchmark_startSend(&benchmark);
            tsgl_display_setBacklight(&display, fmap(sinValue, -1, 1, 64, 255));
            tsgl_display_asyncSend(&display, &framebuffer, &framebuffer2);
            tsgl_benchmark_endSend(&benchmark);
            tsgl_benchmark_print(&benchmark);
        }
        tsgl_display_setBacklight(&display, 255);
    }
}