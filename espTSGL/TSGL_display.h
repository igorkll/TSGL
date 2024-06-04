#pragma once
#include "TSGL.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include "driver/spi_master.h"

typedef struct {
    tsgl_colormode colormode;
    tsgl_pos width;
    tsgl_pos height;
    spi_device_handle_t* spi;
} tsgl_display;

bool tsgl_display_init(tsgl_display* display, tsgl_colormode colormode, tsgl_pos width, tsgl_pos height);
void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer);
void tsgl_display_free(tsgl_display* display);