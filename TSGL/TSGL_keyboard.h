#pragma once
#include "TSGL.h"
#include "TSGL_gui.h"
#include <esp_log.h>
#include <driver/gpio.h>

typedef struct {
    bool highLevel;
    gpio_num_t pin;
} bind_pin;

typedef struct {
    int buttonID;
    bool state;
    bool whenPressed;
    bool whenReleasing;

    tsgl_gui* object;

    uint8_t bindType;
    void* bind;
} bind_state;

typedef struct {
    bind_state** binds;
    size_t bindsCount;
} tsgl_keyboard;

// you can use a symbol as the button ID
void tsgl_keyboard_init(tsgl_keyboard* keyboard);
void tsgl_keyboard_bindButton(tsgl_keyboard* keyboard, int buttonID, bool pull, bool highLevel, gpio_num_t pin);
bind_state* tsgl_keyboard_find(tsgl_keyboard* keyboard, int buttonID);

void tsgl_keyboard_bindToGui(tsgl_keyboard* keyboard, int buttonID, tsgl_gui* object); //allows you to simulate clicking on an element using a button, to work, you need to call readState at the button
bool tsgl_keyboard_readState(tsgl_keyboard* keyboard, int buttonID); //be sure to call before using whenPressed, getState, whenReleasing to update the status
void tsgl_keyboard_readAll(tsgl_keyboard* keyboard); //calls readState on all buttons
bool tsgl_keyboard_getState(tsgl_keyboard* keyboard, int buttonID);
bool tsgl_keyboard_whenPressed(tsgl_keyboard* keyboard, int buttonID);
bool tsgl_keyboard_whenReleasing(tsgl_keyboard* keyboard, int buttonID);
void tsgl_keyboard_free(tsgl_keyboard* keyboard);