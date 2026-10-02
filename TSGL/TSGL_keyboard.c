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
        if (rawState != bindState->rawState) {
            if (rawState) {
                bindState->raw_press_time = time;
            } else {
                bindState->raw_release_time = time;
            }
            bindState->rawState = rawState;
        }

        if (rawState) {
            if (bindState->pressing_ms == 0 || time - bindState->raw_press_time >= bindState->pressing_ms)
               bindState->newState = true;
        } else {
            if (bindState->releasing_ms == 0 || time - bindState->raw_release_time >= bindState->releasing_ms)
                bindState->newState = false;
        }
        
        // ------------------- process press/release trigger
        bindState->whenPressed = false;
        bindState->whenReleasing = false;
        if (bindState->newState != bindState->state) {
            if (bindState->newState) {
                bindState->whenPressed = true;
                bindState->press_time = time;
            } else {
                bindState->whenReleasing = true;
                bindState->release_time = time;
            }
            bindState->state = bindState->newState;
        }

        // ------------------- process gui
        if (bindState->object != NULL) {
            if (bindState->whenPressed) tsgl_gui_processClick(bindState->object, 0, 0, tsgl_gui_click);
            if (bindState->whenReleasing) tsgl_gui_processClick(bindState->object, 0, 0, tsgl_gui_drop);
        }

        // ------------------- process hold
        if (bindState->state) {
            bindState->hold = time - bindState->press_time >= bindState->hold_time_ms;
            bindState->holdWithTrigger = false;
            if (bindState->hold) {
                if (!bindState->newHold) {
                    bindState->hold_time = time;
                    bindState->hold_trigger_time = time;
                    bindState->holdWithTrigger = true;
                    bindState->newHold = true;
                } else if (time - bindState->hold_trigger_time >= bindState->trigger_per_ms) {
                    bindState->hold_trigger_time = time;
                    bindState->holdWithTrigger = true;
                }
            }
        } else {
            bindState->hold = false;
            bindState->newHold = false;
            bindState->holdWithTrigger = false;
            if (!bindState->hold) bindState->unhold_time = time;
        }
    }
    return bindState->state;
}

void tsgl_keyboard_init(tsgl_keyboard* keyboard) {
    memset(keyboard, 0, sizeof(tsgl_keyboard));
}

void tsgl_keyboard_initBindDefaults(tsgl_keyboard_bind* bind) {
    bind->hold_time_ms = TSGL_KEYBOARD_DEFAULT_HOLD_TIME_MS;
    bind->trigger_per_ms = TSGL_KEYBOARD_DEFAULT_TRIGGER_PER_MS;
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
    tsgl_keyboard_initBindDefaults(bindState);

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

void tsgl_keyboard_setHold(tsgl_keyboard* keyboard, int buttonID, time_t hold_time_ms, time_t trigger_per_ms) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    if (bindState != NULL) {
        bindState->hold_time_ms = hold_time_ms;
        bindState->trigger_per_ms = trigger_per_ms;
    } else {
        ESP_LOGE(TAG, "tsgl_keyboard_setHold. there is no button with ID %i/%c", buttonID, (char)buttonID);
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

bool tsgl_keyboard_getRawState(tsgl_keyboard* keyboard, int buttonID) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    if (bindState != NULL) {
        return bindState->rawState;
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

bool tsgl_keyboard_whenHold(tsgl_keyboard* keyboard, int buttonID) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    if (bindState != NULL) {
        return bindState->hold;
    }

    return false;
}

bool tsgl_keyboard_whenHoldWithTrigger(tsgl_keyboard* keyboard, int buttonID) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    if (bindState != NULL) {
        return bindState->holdWithTrigger;
    }

    return false;
}

bool tsgl_keyboard_whenPressedOrHold(tsgl_keyboard* keyboard, int buttonID) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    if (bindState != NULL) {
        return bindState->whenPressed | bindState->hold;
    }

    return false;
}

bool tsgl_keyboard_whenPressedOrHoldWithTrigger(tsgl_keyboard* keyboard, int buttonID) {
    tsgl_keyboard_bind* bindState = tsgl_keyboard_findButton(keyboard, buttonID);
    if (bindState != NULL) {
        return bindState->whenPressed || bindState->holdWithTrigger;
    }

    return false;
}
