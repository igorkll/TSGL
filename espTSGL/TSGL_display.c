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




typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

static const lcd_init_cmd_t st_init_cmds[]={
    /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
    {0x36, {(1<<5)|(1<<6)}, 1},
    /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x05}, 1}, //0x05 / 0x06
    /* Porch Setting */
    {0xB2, {0x0c, 0x0c, 0x00, 0x33, 0x33}, 5},
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    {0xB7, {0x45}, 1},
    /* VCOM Setting, VCOM=1.175V */
    {0xBB, {0x2B}, 1},
    /* LCM Control, XOR: BGR, MX, MH */
    {0xC0, {0x2C}, 1},
    /* VDV and VRH Command Enable, enable=1 */
    {0xC2, {0x01, 0xff}, 2},
    /* VRH Set, Vap=4.4+... */
    {0xC3, {0x11}, 1},
    /* VDV Set, VDV=0 */
    {0xC4, {0x20}, 1},
    /* Frame Rate Control, 60Hz, inversion=0 */
    {0xC6, {0x0f}, 1},
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    {0xD0, {0xA4, 0xA1}, 1},
    /* Positive Voltage Gamma Control */
    {0xE0, {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19}, 14},
    /* Negative Voltage Gamma Control */
    {0xE1, {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19}, 14},
    /* Sleep Out */
    {0x11, {0}, 0x80},
    /* Display On */
    {0x29, {0}, 0x80},
    {0x2A, {0, 0, (320)>>8, (320)&0xff}, 4},
    {0x2B, {0>>8, 0&0xff, 320>>8, 320&0xff}, 4},
    {0x2C, {}, 0},
    {0, {0}, 0xff}
};











esp_err_t tsgl_display_spi(tsgl_display* display, tsgl_pos width, tsgl_pos height, spi_host_device_t spihost, size_t freq, int8_t dc, int8_t cs, int8_t rst) {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = freq,
        .mode = 0,
        .spics_io_num = cs,
        .queue_size = 7,
        .pre_cb = tsgl_spi_pre_transfer_callback
    };

    display->width = width;
    display->height = height;
    display->interfaceType = tsgl_display_interface_spi;
    display->interface = malloc(sizeof(spi_device_handle_t));
    display->dc = dc;

    esp_err_t result = spi_bus_add_device(spihost, &devcfg, (spi_device_handle_t*)display->interface);
    if (result == ESP_OK) {
        // configuration of non-SPI pins
        gpio_config_t io_conf = {};
        if (dc >= 0) io_conf.pin_bit_mask |= 1ULL << dc;
        if (rst >= 0) io_conf.pin_bit_mask |= 1ULL << rst;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_up_en = true;
        gpio_config(&io_conf);

        // reset display
        if (rst >= 0) {
            gpio_set_level(rst, false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_set_level(rst, true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        // init display
        uint16_t cmd = 0;
        while (st_init_cmds[cmd].databytes!=0xff) {
            tsgl_spi_sendCommand(display, st_init_cmds[cmd].cmd);
            tsgl_spi_sendData(display, st_init_cmds[cmd].data, st_init_cmds[cmd].databytes&0x1F);
            if (st_init_cmds[cmd].databytes&0x80) {
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            cmd++;
        }
    } else {
        free(display->interface);
    }
    return result;
}

void tsgl_display_send(tsgl_display* display, tsgl_framebuffer* framebuffer) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            tsgl_spi_sendData(display, framebuffer->buffer, framebuffer->buffersize);
            break;
    }
}

void tsgl_display_free(tsgl_display* display) {
    switch (display->interfaceType) {
        case tsgl_display_interface_spi:
            spi_bus_remove_device(*((spi_device_handle_t*)display->interface));
            break;
    }
    free(display->interface);
}