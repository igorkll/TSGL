#pragma once
#include "TSGL.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>

typedef struct {
    tsgl_pos width;
    tsgl_pos height;
    uint8_t interfaceType;
    void* interface;
} tsgl_display;

bool tsgl_display_initSpi(tsgl_display* display, tsgl_pos width, tsgl_pos height, spi_bus_config_t* buscfg, int8_t dc, int8_t cs, int8_t rst);
void tsgl_display_select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos sx, tsgl_pos sy);
void tsgl_display_all(tsgl_display* display); //cancels the select action
void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer);
void tsgl_display_flood(tsgl_display* display, tsgl_color color, uint32_t count);
void tsgl_display_free(tsgl_display* display);