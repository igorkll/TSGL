#if __has_include(<driver/dac.h>)
#include "TSGL_sound.h"
#include "TSGL_filesystem.h"
#include <stdio.h>
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/soc.h>
#include <esp_log.h>

static const char* TAG = "TSGL_sound";

static void IRAM_ATTR _timer_ISR(void* _sound) {
    tsgl_sound* sound = _sound;
    if (sound->position >= sound->len) {
        sound->position = 0;
        if (sound->loop) {
        } else {
            tsgl_sound_stop(sound);
        }
    } else {
    }
    dac_oneshot_output_voltage(sound->channel1, *(((uint8_t*)sound->data) + sound->position));
    sound->position += sound->bit_rate;
}

static void _pushTask(void* _sound) {
    tsgl_sound* sound = _sound;
    while (sound->playing) vTaskDelay(1);
    tsgl_sound_free(sound);
    free(sound);
    vTaskDelete(NULL);
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
    sound->data = malloc(sound->len);
    fread(sound->data, sound->bit_rate, sound->len, file);
    fclose(file);
    return ESP_OK;
}

void tsgl_sound_play(tsgl_sound* sound, tsgl_sound_channel channel1, tsgl_sound_channel channel2) {
    if (sound->playing) {
        ESP_LOGW(TAG, "tsgl_sound_play skipped. the track is already playing");
        return;
    }

    if (channel1 >= 0) {
        dac_oneshot_config_t conf = {
            .chan_id = channel1
        };

        ESP_ERROR_CHECK_WITHOUT_ABORT(dac_oneshot_new_channel(&conf, &sound->channel1));
        sound->channel1Bool = true;
    } else {
        sound->channel1Bool = false;
    }
    
    if (channel2 >= 0) {
        dac_oneshot_config_t conf = {
            .chan_id = channel2
        };
        
        ESP_ERROR_CHECK_WITHOUT_ABORT(dac_oneshot_new_channel(&conf, &sound->channel2));
        sound->channel2Bool = true;
    } else {
        sound->channel2Bool = false;
    }
    
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
    sound->playing = true;

	ESP_ERROR_CHECK(timer_init(sound->timerGroup, sound->timer, &config));
	ESP_ERROR_CHECK(timer_set_counter_value(sound->timerGroup, sound->timer, 0x00000000ULL));
	ESP_ERROR_CHECK(timer_set_alarm_value(sound->timerGroup, sound->timer, APB_CLK_FREQ / config.divider / sound->sample_rate));
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
    if (sound->channel1Bool) ESP_ERROR_CHECK_WITHOUT_ABORT(dac_oneshot_del_channel(sound->channel1));
    if (sound->channel2Bool) ESP_ERROR_CHECK_WITHOUT_ABORT(dac_oneshot_del_channel(sound->channel2));
    sound->playing = false;
}

void tsgl_sound_free(tsgl_sound* sound) {
    if (sound->playing) tsgl_sound_stop(sound);
    free(sound->data);
}

void tsgl_sound_push_pcm(const char* path, float speed, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_channel channel1, tsgl_sound_channel channel2) {
    tsgl_sound* sound = malloc(sizeof(tsgl_sound));
    tsgl_sound_load_pcm(sound, path, sample_rate, bit_rate, channels);
    sound->speed = speed;
    tsgl_sound_play(sound, channel1, channel2);
    xTaskCreate(_pushTask, NULL, 8096, &sound, 1, NULL);
}

#endif