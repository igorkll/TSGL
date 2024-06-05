#pragma once
#include <driver/spi_master.h>
#include "TSGL_framebuffer.h"

// recommended SPI pins to get the maximum frequency, if you change at least one pin, the maximum frequency will drop to 20 megahertz
#ifdef CONFIG_IDF_TARGET_ESP32
    #define TSGL_HOST1      SPI2_HOST
    #define TSGL_HOST1_MISO 12
    #define TSGL_HOST1_MOSI 13
    #define TSGL_HOST1_CLK  14
    #define TSGL_HOST1_CS   15
    #define TSGL_HOST1_QUADWP 2
    #define TSGL_HOST1_QUADHD 4

    #define TSGL_HOST1      SPI3_HOST
    #define TSGL_HOST1_MISO 19
    #define TSGL_HOST1_MOSI 23
    #define TSGL_HOST1_CLK  18
    #define TSGL_HOST1_CS   5
    #define TSGL_HOST1_QUADWP 22
    #define TSGL_HOST1_QUADHD 21
#if defined CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3

#elif defined CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32C6

#elif defined CONFIG_IDF_TARGET_ESP32H2

#endif

void tsgl_spi_pre_transfer_callback(spi_transaction_t* t);
void tsgl_spi_sendCommand(tsgl_display* display, const uint8_t cmd);
void tsgl_spi_sendData(tsgl_display* display, const uint8_t* data, size_t size);