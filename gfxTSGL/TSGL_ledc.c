#include <driver/ledc.h>
#include <driver/gpio.h>
#include <math.h>

static uint8_t _getLedcChannel() {
    static int8_t ledcChannel = -1;
    ledcChannel++;
    if (ledcChannel >= 8) ledcChannel = 0;
    return ledcChannel;
}

static uint8_t CRTValue(uint8_t val) {
    return (0.0003066 * pow(val, 2.46));
}

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY          5000
int8_t tsgl_ledc_new(gpio_num_t pin, bool invert) {
    static bool ledcInited = false;
    if (!ledcInited) {
        ledc_timer_config_t ledc_timer = {
            .speed_mode       = LEDC_MODE,
            .timer_num        = LEDC_TIMER,
            .duty_resolution  = LEDC_DUTY_RES,
            .freq_hz          = LEDC_FREQUENCY,
            .clk_cfg          = LEDC_AUTO_CLK
        };

        if (ledc_timer_config(&ledc_timer) != ESP_OK) return -1;
        ledcInited = true;
    }

    int8_t channel = _getLedcChannel();
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = channel,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = pin,
        .duty           = invert ? 255 : 0,
        .hpoint         = 0
    };
    if (ledc_channel_config(&ledc_channel) != ESP_OK) return -1;
    return channel;
}

void tsgl_ledc_set(int8_t channel, bool invert, uint8_t value) {
    if (invert) {
        ledc_set_duty(LEDC_MODE, channel, CRTValue(255 - value));
    } else {
        ledc_set_duty(LEDC_MODE, channel, CRTValue(value));
    }
    ledc_update_duty(LEDC_MODE, channel);
}