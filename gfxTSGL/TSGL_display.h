#pragma once
#include "TSGL.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>

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

esp_err_t tsgl_display_spi(tsgl_display* display, tsgl_pos width, tsgl_pos height, spi_host_device_t spihost, size_t freq, int8_t dc, int8_t cs, int8_t rst);
void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer);
void tsgl_display_free(tsgl_display* display);

void tsgl_display_set(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_rawcolor color);
void tsgl_display_fill(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color);
void tsgl_display_rect(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_rawcolor color);
void tsgl_display_clear(tsgl_display* display, tsgl_rawcolor color);