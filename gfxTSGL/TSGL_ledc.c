#include "TSGL_ledc.h"
#include <driver/ledc.h>
#include <math.h>

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY          5000

int8_t tsgl_ledc_getChannel() {
    static int8_t ledcChannel = -1;
    ledcChannel++;
    if (ledcChannel >= 8) ledcChannel = 0;
    return ledcChannel;
}

uint8_t tsgl_ledc_CRTValue(uint8_t val) {
    return 0.0003066 * pow(val, 2.46);
}

esp_err_t tsgl_ledc_new(tsgl_ledc* obj, gpio_num_t pin, bool invert, uint8_t defaultValue) {
    static bool ledcInited = false;
    if (!ledcInited) {
        ledc_timer_config_t ledc_timer = {
            .speed_mode       = LEDC_MODE,
            .timer_num        = LEDC_TIMER,
            .duty_resolution  = LEDC_DUTY_RES,
            .freq_hz          = LEDC_FREQUENCY,
            .clk_cfg          = LEDC_AUTO_CLK
        };

        ledc_timer_config(&ledc_timer); //there is no check here because the timer can already be initialized by the user with other settings
        ledcInited = true;
    }

    int8_t channel = tsgl_ledc_getChannel();
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = channel,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = pin,
        .duty           = invert ? (255 - tsgl_ledc_CRTValue(defaultValue)) : tsgl_ledc_CRTValue(defaultValue),
        .hpoint         = 0
    };
    
    obj->channel = channel;
    obj->invert = invert;
    return ledc_channel_config(&ledc_channel);
}

void tsgl_ledc_set(tsgl_ledc* obj, uint8_t value) {
    if (obj->invert) {
        ledc_set_duty(LEDC_MODE, obj->channel, 255 - tsgl_ledc_CRTValue(value));
    } else {
        ledc_set_duty(LEDC_MODE, obj->channel, tsgl_ledc_CRTValue(value));
    }
    ledc_update_duty(LEDC_MODE, obj->channel);
}

void tsgl_ledc_free(tsgl_ledc* obj) {
    tsgl_ledc_set(obj, 0);
}