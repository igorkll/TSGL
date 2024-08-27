#pragma once
#include "TSGL.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include "TSGL_ledc.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_lcd_io_spi.h>
#include <esp_lcd_panel_io.h>

typedef enum {
    tsgl_display_interface_spi,
    tsgl_display_interface_lcd
} tsgl_display_interfaceType;

typedef enum {
    tsgl_display_init_none = 0,
    tsgl_display_init_color,
    tsgl_display_init_framebuffer
} tsgl_display_init;

typedef struct {
    spi_device_handle_t* spi;
    int8_t dc;
} tsgl_display_interfaceData_spi;

typedef struct {
    esp_lcd_panel_io_handle_t* lcd;
} tsgl_display_interfaceData_lcd;

typedef struct {
    const tsgl_driver* driver;
    bool invertBacklight;
    bool invert;
    bool swapRGB;
    bool flipX;
    bool flipY;
    bool flipXY;
    tsgl_pos width;
    tsgl_pos height;
    tsgl_pos offsetX; //on many displays, the visible area does not start from the beginning
    tsgl_pos offsetY;

    // the first state after initialization
    tsgl_display_init init_state;
    tsgl_rawcolor init_color;

    const uint8_t* init_framebuffer_ptr;
    size_t init_framebuffer_size;
    uint8_t init_framebuffer_rotation;

    bool backlight_init;
    gpio_num_t backlight_pin;
    uint8_t backlight_value;
} tsgl_display_settings;

typedef struct { //please DO NOT write anything in the fields of the structure
    tsgl_driver_storage storage;
    tsgl_pos width;
    tsgl_pos height;
    tsgl_pos defaultWidth;
    tsgl_pos defaultHeight;
    uint8_t rotation;
    tsgl_colormode colormode;
    float colorsize;
    tsgl_rawcolor black;
    bool enable;
    bool invert;
    bool baseInvert;
    tsgl_pos offsetX;
    tsgl_pos offsetY;
    bool invertBacklight;

    tsgl_ledc* backlight;

    //bool softwareInvert; //it can cause severe lags, it is used to invert colors to displays that do not support this hardware

    const tsgl_driver* driver;
    tsgl_display_interfaceType interfaceType;
    void* interface;
} tsgl_display;

// ---------------- initializing the display
esp_err_t tsgl_display_spi(tsgl_display* display, const tsgl_display_settings settings, spi_host_device_t spihost, size_t freq, gpio_num_t dc, gpio_num_t cs, gpio_num_t rst);
void tsgl_display_free(tsgl_display* display);

// ---------------- backlight
//on some displays, brightness control works without this, but on others you need to use a separate pin.
//it is better to initialize the backlight BEFORE initializing the screen so that the initialization process is not visible,
//but calling tsgl_display_attachBacklight before initialization display will cause tsgl_display_setBacklight to malfunction.
//therefore, to initialize the backlight, before initializing the screen, add the backlight pin and the standard state in the display initialization settings and raise the appropriate flag
esp_err_t tsgl_display_attachBacklight(tsgl_display* display, gpio_num_t pin, uint8_t defaultValue);
void tsgl_display_setBacklight(tsgl_display* display, uint8_t value); //changes the display backlight states using the appropriate registers specified in the driver. if the Q method is called, it uses hardware backlight control instead of registers

// ---------------- low level methods (it is not recommended to use)
void tsgl_display_sendCommand(tsgl_display* display, const uint8_t command);
void tsgl_display_sendData(tsgl_display* display, const uint8_t* data, size_t size);
void tsgl_display_sendCommandWithArg(tsgl_display* display, const uint8_t command, const uint8_t arg);
void tsgl_display_sendCommandWithArgs(tsgl_display* display, const uint8_t command, const uint8_t* args, size_t argsCount);
void tsgl_display_sendFlood(tsgl_display* display, const uint8_t* data, size_t size, size_t flood);

// ---------------- control
void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer);
void tsgl_display_asyncSend(tsgl_display* display, tsgl_framebuffer* framebuffer, tsgl_framebuffer* framebuffer2); //sends the framebuffer asynchronously and swaps buffers. it requires a complete redrawing of the buffer for correct operation. both buffers must be initialized in the same way
void tsgl_display_asyncCopySend(tsgl_display* display, tsgl_framebuffer* framebuffer, tsgl_framebuffer* framebuffer2); //it does not swapbuffers, but copies buffer 1 to buffer 2 before sending it. it is slow but does not require a complete redrawing of the buffer. the problem is that it is OFTEN slower than just using tsgl_display_send

void tsgl_display_rotate(tsgl_display* display, uint8_t rotation); //it is not recommended to use this method when working with framebuffer (or use with tsgl_framebuffer_hardwareRotate)

void tsgl_display_select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height);
void tsgl_display_selectAll(tsgl_display* display);
void tsgl_display_selectIfNeed(tsgl_display* display); //calls selectAll if is a driver said that the display resets the area after each command. it should be called after tsgl_display_sendCommand for compatibility with such displays

void tsgl_display_setEnable(tsgl_display* display, bool state); //the display is on by default. however, if you have backlight control enabled, then to activate it, you need to call this method during initialization
void tsgl_display_setInvert(tsgl_display* display, bool state);

// ---------------- graphic (these methods reset the selected area)
void tsgl_display_push(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_sprite* sprite);
void tsgl_display_setWithoutCheck(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color);
void tsgl_display_set(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color);
void tsgl_display_line(tsgl_display* display, tsgl_pos x1, tsgl_pos y1, tsgl_pos x2, tsgl_pos y2, tsgl_rawcolor color, tsgl_pos stroke);
void tsgl_display_fill(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color);
void tsgl_display_fillWithoutCheck(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color);
void tsgl_display_rect(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color, tsgl_pos stroke);
tsgl_print_textArea tsgl_display_text(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_print_settings sets, const char* text);
void tsgl_display_clear(tsgl_display* display, tsgl_rawcolor color);