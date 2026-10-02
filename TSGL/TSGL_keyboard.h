#pragma once
#include "TSGL.h"
#include "TSGL_gui.h"
#include <esp_log.h>
#include <driver/gpio.h>

#define TSGL_KEYBOARD_DEFAULT_HOLD_TIME_MS 1000
#define TSGL_KEYBOARD_DEFAULT_TRIGGER_PER_MS 100

typedef struct {
    int buttonID;
    bool state;
    bool whenPressed;
    bool whenReleasing;

    tsgl_gui* object;

    uint8_t bindType;
    void* bind;

    time_t pressing_ms;
    time_t releasing_ms;

    time_t raw_press_time;
    time_t raw_release_time;
    
    time_t press_time;
    time_t release_time;

    bool rawState;
    bool newState;

    time_t hold_time_ms;
    time_t trigger_per_ms;

    bool hold;
    bool newHold;
    bool holdWithTrigger;
    time_t hold_time;
    time_t unhold_time;
    time_t hold_trigger_time;
} tsgl_keyboard_bind;

typedef struct {
    tsgl_keyboard_bind** binds;
    size_t bindsCount;
} tsgl_keyboard;

// you can use a char as the button ID
void tsgl_keyboard_init(tsgl_keyboard* keyboard);
void tsgl_keyboard_free(tsgl_keyboard* keyboard);

tsgl_keyboard_bind* tsgl_keyboard_bindButton(tsgl_keyboard* keyboard, int buttonID, bool pull, bool highLevel, gpio_num_t pin);
tsgl_keyboard_bind* tsgl_keyboard_findButton(tsgl_keyboard* keyboard, int buttonID);
void tsgl_keyboard_initBindDefaults(tsgl_keyboard_bind* bind);
bool tsgl_keyboard_unbindButton(tsgl_keyboard* keyboard, int buttonID);

void tsgl_keyboard_readAll(tsgl_keyboard* keyboard); //calls readState on all buttons
void tsgl_keyboard_bindToGui(tsgl_keyboard* keyboard, int buttonID, tsgl_gui* object); //allows you to simulate clicking on an element using a button, to work, you need to call readState at the button
void tsgl_keyboard_setDebounce(tsgl_keyboard* keyboard, int buttonID, time_t pressing_ms, time_t releasing_ms);
void tsgl_keyboard_setHold(tsgl_keyboard* keyboard, int buttonID, time_t hold_time_ms, time_t trigger_per_ms);

bool tsgl_keyboard_readState(tsgl_keyboard* keyboard, int buttonID); //be sure to call before using whenPressed, getState, whenReleasing to update the status
bool tsgl_keyboard_getState(tsgl_keyboard* keyboard, int buttonID);
bool tsgl_keyboard_getRawState(tsgl_keyboard* keyboard, int buttonID);
bool tsgl_keyboard_whenPressed(tsgl_keyboard* keyboard, int buttonID);
bool tsgl_keyboard_whenReleasing(tsgl_keyboard* keyboard, int buttonID);
bool tsgl_keyboard_whenHold(tsgl_keyboard* keyboard, int buttonID);
bool tsgl_keyboard_whenHoldWithTrigger(tsgl_keyboard* keyboard, int buttonID);
bool tsgl_keyboard_whenPressedOrHold(tsgl_keyboard* keyboard, int buttonID);
bool tsgl_keyboard_whenPressedOrHoldWithTrigger(tsgl_keyboard* keyboard, int buttonID);
