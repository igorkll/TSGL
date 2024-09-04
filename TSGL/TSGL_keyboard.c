#include "TSGL_keyboard.h"
#include <string.h>

static const char* TAG = "TSGL_keyboard";

typedef struct {
    bool highLevel;
    gpio_num_t pin;
} _bind_pin;

static bool _rawRead(tsgl_keyboard_bind* bindState) {
    bool rawState = false;
    if (bindState != NULL) {
        switch (bindState->bindType) {
            case 0:
                _bind_pin* bind = bindState->bind;
                rawState = gpio_get_level(bind->pin);
                if (!bind->highLevel) rawState = !rawState;
                break;

            default:
                ESP_LOGE(TAG, "unknown bind type: %i", bindState->bindType);
                break;
        }

        time_t time = tsgl_time();
        if (rawState != bindState->realState) {
            if (rawState) {
                bindState->press_time = time;
            } else {
                bindState->release_time = time;
            }
            bindState->realState = rawState;
        }

        if (rawState) {
            if (bindState->pressing_ms == 0 || time - bindState->press_time >= bindState->pressing_ms)
                bindState->realState = true;
        } else {
            if (bindState->releasing_ms == 0 || time - bindState->release_time >= bindState->releasing_ms)
                bindState->realState = false;
        }
        
        bindState->whenPressed = false;
        bindState->whenReleasing = false;
        if (bindState->realState != bindState->state) {
            if (bindState->realState) {
                bindState->whenPressed = true;
            } else {
                bindState->whenReleasing = true;
            }
            bindState->state = bindState->realState;
        }

        if (bindState->object != NULL) {
            if (bindState->whenPressed) tsgl_gui_processClick(bindState->object, 0, 0, tsgl_gui_click);
            if (bindState->whenReleasing) tsgl_gui_processClick(bindState->object, 0, 0, tsgl_gui_drop);
        }
    }
    return bindState->state;
}

void tsgl_keyboard_init(tsgl_keyboard* keyboard) {
    memset(keyboard, 0, sizeof(tsgl_keyboard));
}

tsgl_keyboard_bind* tsgl_keyboard_bindButton(tsgl_keyboard* keyboard, int buttonID, bool pull, bool highLevel, gpio_num_t pin) {
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

    _bind_pin* bindPin = calloc(1, sizeof(_bind_pin));
    bindPin->pin = pin;
    bindPin->highLevel = highLevel;

    tsgl_keyboard_bind* bindState = calloc(1, sizeof(tsgl_keyboard_bind));
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

    return bindState;
}

bool tsgl_keyboard_unbindButton(tsgl_keyboard* keyboard, int buttonID) {
    for (size_t i = 0; i < keyboard->bindsCount; i++) {
        tsgl_keyboard_bind* bindState = keyboard->binds[i];
        if (bindState->buttonID == buttonID) {
            keyboard->binds[i] = keyboard->binds[keyboard->bindsCount - 1];
            keyboard->bindsCount--;
            keyboard->binds = realloc(keyboard->binds, keyboard->bindsCount * sizeof(size_t));
            return true;
        }
    }
    ESP_LOGE(TAG, "failed to find button to unbind: %i", buttonID);
    return false;
}

tsgl_keyboard_bind* tsgl_keyboard_findButton(tsgl_keyboard* keyboard, int buttonID) {
    for (size_t i = 0; i < keyboard->bindsCount; i++) {
        tsgl_keyboard_bind* bindState = keyboard->binds[i];
        if (bindState->buttonID == buttonID) {
            return bindState;
        }
    }
    ESP_LOGE(TAG, "failed to find button: %i", buttonID);
    return NULL;
}

void tsgl_keyboard_bindToGui(tsgl_keyboard* keyboard, int buttonID, tsgl_gui* object) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    if (bindState != NULL) {
        bindState->object = object;
    } else {
        ESP_LOGE(TAG, "tsgl_keyboard_bindToGui. there is no button with ID %i/%c", buttonID, (char)buttonID);
    }
}

void tsgl_keyboard_setDebounce(tsgl_keyboard* keyboard, int buttonID, time_t pressing_ms, time_t releasing_ms) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    if (bindState != NULL) {
        bindState->pressing_ms = pressing_ms;
        bindState->releasing_ms = releasing_ms;
    } else {
        ESP_LOGE(TAG, "tsgl_keyboard_setDebounce. there is no button with ID %i/%c", buttonID, (char)buttonID);
    }
}

bool tsgl_keyboard_readState(tsgl_keyboard* keyboard, int buttonID) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    return _rawRead(bindState);
}

void tsgl_keyboard_readAll(tsgl_keyboard* keyboard) {
    for (size_t i = 0; i < keyboard->bindsCount; i++) {
        _rawRead(keyboard->binds[i]);
    }
}

bool tsgl_keyboard_getState(tsgl_keyboard* keyboard, int buttonID) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    if (bindState != NULL) {
        return bindState->state;
    }
    return false;
}

bool tsgl_keyboard_whenPressed(tsgl_keyboard* keyboard, int buttonID) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    if (bindState != NULL) {
        return bindState->whenPressed;
    }
    return false;
}

bool tsgl_keyboard_whenReleasing(tsgl_keyboard* keyboard, int buttonID) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    if (bindState != NULL) {
        return bindState->whenReleasing;
    }
    return false;
}

void tsgl_keyboard_free(tsgl_keyboard* keyboard) {
    for (size_t i = 0; i < keyboard->bindsCount; i++) {
        tsgl_keyboard_bind* bindState = keyboard->binds[i];
        free(bindState->bind);
        free(bindState);
    }
    free(keyboard->binds);
}