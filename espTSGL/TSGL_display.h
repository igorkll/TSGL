#pragma once
#include "TSGL.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>

typedef enum {
    tsgl_display_interface_spi
} tsgl_display_interfaceType;

typedef struct {
    tsgl_pos width;
    tsgl_pos height;
    tsgl_display_interfaceType interfaceType;
    void* interface;
    int8_t dc;
} tsgl_display;

bool tsgl_display_spi(tsgl_display* display, tsgl_pos width, tsgl_pos height, spi_host_device_t spihost, size_t freq, int8_t dc, int8_t cs, int8_t rst);
bool tsgl_display_initSpi(tsgl_display* display, tsgl_pos width, tsgl_pos height, spi_host_device_t spihost, size_t freq, int8_t dc, int8_t cs, int8_t rst, int8_t miso, int8_t mosi, int8_t sclk);
void tsgl_display_select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos sx, tsgl_pos sy);
void tsgl_display_all(tsgl_display* display); //cancels the select action
void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer);
void tsgl_display_free(tsgl_display* display);