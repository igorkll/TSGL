#pragma once
#include "TSGL.h"
#include <drivers/gpio.h>

typedef struct {
    bool oldState;
    bool whenPressed;
    bool whenReleasing;
    int buttonID;
    bool highLevel;
} bind_state;

typedef struct {
    bind_state** binds;
    size_t bindsCount;
} tsgl_keyboard;

// you can use a symbol as the button ID
void tsgl_keyboard_init(tsgl_keyboard* keyboard);
void tsgl_keyboard_bindButton(tsgl_keyboard* keyboard, int buttonID, bool pull, bool highLevel, gpio_num_t pin);
bool tsgl_keyboard_getState(tsgl_keyboard* keyboard, int buttonID); //be sure to call before using whenPressed and whenReleasing to update the status
bool tsgl_keyboard_whenPressed(tsgl_keyboard* keyboard, int buttonID);
bool tsgl_keyboard_whenReleasing(tsgl_keyboard* keyboard, int buttonID);
void tsgl_keyboard_free(tsgl_keyboard* keyboard);