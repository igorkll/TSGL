#if __has_include(<driver/dac.h>)
#include "TSGL_sound.h"
#include "TSGL_filesystem.h"
#include <driver/timer.h>
#include <stdio.h>
#include <esp_attr.h>

static const char* TAG = "TSGL_sound";

static void IRAM_ATTR _timer_ISR(void* _sound) {
    tsgl_sound* sound = _sound;
    timer_group_clr_intr_status_in_isr(sound->timerGroup, sound->timer);
    if (sound->position >= sound->len) {
        sound->position = 0;
        if (sound->loop) {
            timer_group_enable_alarm_in_isr(sound->timerGroup, sound->timer);
        } else {
            sound->playing = false;
        }
    } else {
        timer_group_enable_alarm_in_isr(sound->timerGroup, sound->timer);
    }
    dac_output_voltage(DAC_CHANNEL_1, *(((uint8_t*)sound->data) + sound->position));
    sound->position += sound->bit_rate;
}

esp_err_t tsgl_sound_load_pcm(tsgl_sound* sound, const char* path, size_t sample_rate, size_t bit_rate, size_t channels) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) return ESP_FAIL;
    sound->playing = false;
    sound->pause = false;
    sound->position = 0;
    sound->speed = 1.0;
    sound->len = tsgl_filesystem_getFileSize(path);
    sound->sample_rate = sample_rate;
    sound->bit_rate = bit_rate;
    sound->channels = channels;
    fread(sound->data, sound->bit_rate, sound->len, file);
    fclose(file);
    return ESP_OK;
}

void tsgl_sound_play(tsgl_sound* sound, dac_channel_t channel) {
    if (sound->playing) {
        ESP_LOGW(TAG, "tsgl_sound_play skipped. the track is already playing");
        return;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(dac_output_enable(channel));
    sound->playing = true;
    sound->channel = channel;

    timer_config_t config = {
		.divider = 8,
		.counter_dir = TIMER_COUNT_UP,
		.counter_en = TIMER_PAUSE,
		.alarm_en = TIMER_ALARM_EN,
		.intr_type = TIMER_INTR_LEVEL,
		.auto_reload = 1
	};

    sound->timerGroup = TIMER_GROUP_0;
    sound->timer = TIMER_0;

	ESP_ERROR_CHECK(timer_init(sound->timerGroup, sound->timer, &config));
	ESP_ERROR_CHECK(timer_set_counter_value(sound->timerGroup, sound->timer, 0x00000000ULL));
	ESP_ERROR_CHECK(timer_set_alarm_value(sound->timerGroup, sound->timer, TIMER_BASE_CLK / config.divider / sound_wav_info.sampleRate));
	ESP_ERROR_CHECK(timer_enable_intr(sound->timerGroup, sound->timer));
	timer_isr_register(sound->timerGroup, sound->timer, _timer_ISR, &sound, ESP_INTR_FLAG_IRAM, NULL);
	timer_start(sound->timerGroup, sound->timer);
}

void tsgl_sound_stop(tsgl_sound* sound) {
    if (!sound->playing) {
        ESP_LOGW(TAG, "tsgl_sound_stop skipped. the track is not playing");
        return;
    }

    timer_pause(sound->timerGroup, sound->timer);
    ESP_ERROR_CHECK_WITHOUT_ABORT(dac_output_disable(sound->channel));
    sound->playing = false;
}

void tsgl_sound_free(tsgl_sound* sound) {
    free(sound->data);
}

#endif