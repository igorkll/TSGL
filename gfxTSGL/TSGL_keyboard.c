#include "TSGL_keyboard.h"

void tsgl_keyboard_init(tsgl_keyboard* keyboard) {
    memset(keyboard, 0, sizeof(tsgl_keyboard));
}

void tsgl_keyboard_bindButton(tsgl_keyboard* keyboard, int buttonID, bool pull, bool highLevel, gpio_num_t pin) {
    gpio_config_t io_conf = {0};
    io_conf.pin_bit_mask |= 1ULL << pin;
    io_conf.mode = GPIO_MODE_INPUT;
    if (pull) {
        if (highLevel) {
            io_conf.pull_down_en = true;
        } else {
            io_conf.pull_up_en = true;
        }
    }
    gpio_config(&io_conf);

    bind_state* bindState = calloc(1, sizeof(bind_state));
    bindState->highLevel = highLevel;
    bindState->buttonID = buttonID;

    keyboard->binds[keyboard->bindsCount++] = bindState;
}

bool tsgl_keyboard_getState(tsgl_keyboard* keyboard, int buttonID) {

}

bool tsgl_keyboard_whenPressed(tsgl_keyboard* keyboard, int buttonID) {

}

bool tsgl_keyboard_whenReleasing(tsgl_keyboard* keyboard, int buttonID) {

}

void tsgl_keyboard_free(tsgl_keyboard* keyboard) {
    for (size_t i = 0; i < keyboard->bindsCount; i++) {
        free(keyboard->binds[i]);
    }
}