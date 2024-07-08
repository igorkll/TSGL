#if __has_include(<driver/dac.h>)
#include "TSGL_sound.h"
#include "TSGL_filesystem.h"
#include <stdio.h>
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <esp_heap_caps.h>
#include <freertos/task.h>
#include <soc/soc.h>
#include <esp_log.h>
#include <string.h>

static const char* TAG = "TSGL_sound";

static void IRAM_ATTR _timer_ISR(void* _sound) {
    tsgl_sound* sound = _sound;
    timer_group_clr_intr_status_in_isr(sound->timerGroup, sound->timer);
    if (sound->position >= sound->len) {
        sound->position = 0;
        if (sound->loop) {
            timer_group_enable_alarm_in_isr(sound->timerGroup, sound->timer);
        } else {
            tsgl_sound_stop(sound);
        }
    } else {
        timer_group_enable_alarm_in_isr(sound->timerGroup, sound->timer);
    }
    if (sound->channel1Bool)
        dac_oneshot_output_voltage(sound->channel1, *(((uint8_t*)sound->data) + sound->position));
    if (sound->channel2Bool)
        dac_oneshot_output_voltage(sound->channel2, *(((uint8_t*)sound->data) + sound->position + (sound->channels > 1 ? sound->bit_rate : 0)));
    sound->position += sound->bit_rate * sound->channels;
}

static void _pushTask(void* _sound) {
    tsgl_sound* sound = _sound;
    while (sound->playing) vTaskDelay(1);
    tsgl_sound_free(sound);
    free(sound);
    vTaskDelete(NULL);
}

static void _freeOutputs(tsgl_sound* sound) {
    if (sound->freeOutputs) {
        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_freeOutput(sound->outputs[i]);
        }
    }
    free(sound->outputs);
}

esp_err_t tsgl_sound_load_pcm(tsgl_sound* sound, const char* path, size_t sample_rate, size_t bit_rate, size_t channels) {
    memset(sound, 0, sizeof(tsgl_sound));
    FILE* file = fopen(path, "rb");
    if (file == NULL) return ESP_FAIL;
    sound->speed = 1.0;
    sound->len = tsgl_filesystem_getFileSize(path);
    sound->sample_rate = sample_rate;
    sound->bit_rate = bit_rate;
    sound->channels = channels;
    sound->data = heap_caps_malloc(sound->len, MALLOC_CAP_SPIRAM);
    if (sound->data == NULL) {
        sound->data = malloc(sound->len);
    }
    if (sound->data == NULL) {
        ESP_LOGE(TAG, "the buffer for the sound could not be allocated: %iKB", sound->len / 1024);
    } else {
        fread(sound->data, sound->bit_rate, sound->len, file);
    }
    fclose(file);
    return ESP_OK;
}

void tsgl_sound_setOutput(tsgl_sound* sound, tsgl_sound_output** outputs, size_t outputsCount, bool freeOutputs) {
    _freeOutputs(sound);
    sound->outputsCount = outputsCount;
    sound->outputs = malloc(outputsCount * sizeof(size_t));
    for (size_t i = 0; i < sound->outputsCount; i++) {
        sound->outputs[i] = outputs[i];
    }
    sound->freeOutputs = freeOutputs;
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
	timer_isr_register(sound->timerGroup, sound->timer, _timer_ISR, sound, ESP_INTR_FLAG_IRAM, NULL);
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
    if (sound->data != NULL) {
        free(sound->data);
        sound->data = NULL;
    }
    _freeOutputs(sound);
}

#ifdef HARDWARE_DAC
    tsgl_sound_output* tsgl_sound_newDacOutput(dac_channel_t channel) {
        tsgl_sound_output* output = calloc(1, sizeof(tsgl_sound_output));
        output->channel = calloc(1, sizeof(dac_oneshot_handle_t));
        dac_oneshot_config_t conf = {
            .chan_id = channel
        };
        if (ESP_ERROR_CHECK_WITHOUT_ABORT(dac_oneshot_new_channel(&conf, output->channel)) != ESP_OK) {
            free(output->channel);
            output->channel = NULL;
        }
        return output;
    }
#endif

void tsgl_sound_setOutput(tsgl_sound_output* output, uint8_t* value, size_t bit_rate) {
    #ifdef HARDWARE_DAC
        if (output->channel != NULL) {
            dac_oneshot_output_voltage(output->channel, value);
        }
    #endif
}

void tsgl_sound_freeOutput(tsgl_sound_output* output) {
    #ifdef HARDWARE_DAC
        if (output->channel != NULL) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(dac_oneshot_del_channel(*output->channel));
            free(output->channel);
        }
    #endif
    free(output);
}

#endif