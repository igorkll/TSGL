#include "TSGL.h"
#include "TSGL_display.h"
#include "TSGL_color.h"
#include "TSGL_framebuffer.h"
#include "TSGL_spi.h"

#include <esp_system.h>
#include <esp_err.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

bool tsgl_display_initSpi(tsgl_display* display, tsgl_pos width, tsgl_pos height, spi_bus_config_t* buscfg, int8_t dc, int8_t cs, int8_t rst) {
    display->width = width;
    display->height = height;
    display->interfaceType = tsgl_display_interface_spi;
    
    display->dc = dc;
}

void tsgl_display_select(tsgl_display* display, tsgl_pos x, tsgl_pos y, tsgl_pos sx, tsgl_pos sy) {
    
}

void tsgl_display_all(tsgl_display* display) {

}

void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer) {

}

void tsgl_display_flood(tsgl_display* display, tsgl_color color, uint32_t count) {
    
}

void tsgl_display_free(tsgl_display* display) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            spi_bus_remove_device(*display->interface);
            free(display->interface);
            break;
    }
}