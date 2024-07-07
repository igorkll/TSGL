#include "TSGL_keyboard.h"
#include <string.h>

static const char* TAG = "TSGL_keyboard";

static bool _rawRead(bind_state* bindState) {
    bool state = false;
    if (bindState != NULL) {
        switch (bindState->bindType) {
            case 0:
                bind_pin* bind = bindState->bind;
                state = gpio_get_level(bind->pin);
                if (!bind->highLevel) state = !state;
                break;

            default:
                ESP_LOGE(TAG, "unknown bind type: %i", bindState->bindType);
                break;
        }
        bindState->whenPressed = false;
        bindState->whenReleasing = false;
        if (state != bindState->state) {
            if (state) bindState->whenPressed = true;
            if (!state) bindState->whenReleasing = true;
            bindState->state = state;
        }
    }
    return state;
}

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

    bind_pin* bindPin = calloc(1, sizeof(bind_pin));
    bindPin->pin = pin;
    bindPin->highLevel = highLevel;

    bind_state* bindState = calloc(1, sizeof(bind_state));
    bindState->bind = bindPin;
    bindState->bindType = 0;
    bindState->buttonID = buttonID;

    keyboard->bindsCount++;
    if (keyboard->binds == NULL) {
        keyboard->binds = malloc(keyboard->bindsCount * sizeof(size_t));
    } else {
        keyboard->binds = realloc(keyboard->binds, keyboard->bindsCount * sizeof(size_t));
    }
    keyboard->binds[keyboard->bindsCount - 1] = bindState;
}

bind_state* tsgl_keyboard_find(tsgl_keyboard* keyboard, int buttonID) {
    for (size_t i = 0; i < keyboard->bindsCount; i++) {
        bind_state* bindState = keyboard->binds[i];
        if (bindState->buttonID == buttonID) {
            return bindState;
        }
    }
    ESP_LOGE(TAG, "failed to find button: %i", buttonID);
    return NULL;
}

void tsgl_keyboard_guiBind(tsgl_keyboard* keyboard, int buttonID, tsgl_gui* object) {
    bind_state* bindState = tsgl_keyboard_find(keyboard, buttonID);
    if (bindState != NULL) {
        bindState->object = object;
    }
}

bool tsgl_keyboard_readState(tsgl_keyboard* keyboard, int buttonID) {
    bind_state* bindState = tsgl_keyboard_find(keyboard, buttonID);
    return _rawRead(bindState);
}

void tsgl_keyboard_readAll(tsgl_keyboard* keyboard) {
    for (size_t i = 0; i < keyboard->bindsCount; i++) {
        _rawRead(keyboard->binds[i]);
    }
}

bool tsgl_keyboard_getState(tsgl_keyboard* keyboard, int buttonID) {
    bind_state* bindState = tsgl_keyboard_find(keyboard, buttonID);
    if (bindState != NULL) {
        return bindState->state;
    }
    return false;
}

bool tsgl_keyboard_whenPressed(tsgl_keyboard* keyboard, int buttonID) {
    bind_state* bindState = tsgl_keyboard_find(keyboard, buttonID);
    if (bindState != NULL) {
        return bindState->whenPressed;
    }
    return false;
}

bool tsgl_keyboard_whenReleasing(tsgl_keyboard* keyboard, int buttonID) {
    bind_state* bindState = tsgl_keyboard_find(keyboard, buttonID);
    if (bindState != NULL) {
        return bindState->whenReleasing;
    }
    return false;
}

void tsgl_keyboard_free(tsgl_keyboard* keyboard) {
    for (size_t i = 0; i < keyboard->bindsCount; i++) {
        bind_state* bindState = keyboard->binds[i];
        free(bindState->bind);
        free(bindState);
    }
    free(keyboard->binds);
}