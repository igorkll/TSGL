#include "TSGL_sound.h"
#include "TSGL_filesystem.h"
#include "TSGL_math.h"
#include <esp_attr.h>
#include <esp_heap_caps.h>
#include <soc/soc.h>
#include <esp_log.h>
#include <string.h>

static const char* TAG = "TSGL_sound";
static uint32_t cp0_regs[18];

static uint8_t IRAM_ATTR convertPcm(tsgl_sound* sound, void* source) {
    switch (sound->pcm_format) {
        case tsgl_sound_pcm_unsigned:
            return *((uint8_t*)source);
        
        case tsgl_sound_pcm_signed:
            return *((int8_t*)source) + 128;
    }
    return 0;
}

static void _soundTask(void* _sound) {
    tsgl_sound* sound = _sound;
    fread(sound->buffer, sound->bit_rate, sound->bufferSize, sound->file);
    vTaskSuspend(NULL);

    while (true) {
        ESP_ERROR_CHECK(gptimer_stop(sound->timer));
        fread(sound->buffer, sound->bit_rate, sound->bufferSize, sound->file);
        ESP_ERROR_CHECK(gptimer_start(sound->timer));

        vTaskDelay(1);
        vTaskSuspend(NULL);
    }
}

static bool IRAM_ATTR _timer_ISR(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx) {
    gptimer_stop(timer);
    
    tsgl_sound* sound = user_ctx;
    printf("LOLZ\n");
    //timer_group_clr_intr_status_in_isr(sound->timerGroup, sound->timer);

    size_t dataOffset = sound->position % sound->bufferSize;
    if (sound->file && dataOffset == 0 && sound->soundTask) {
        uint8_t* t = sound->data;
        sound->data = sound->buffer;
        sound->buffer = t;
        xTaskResumeFromISR(sound->task);
    }

    uint8_t* ptr = sound->data + dataOffset;
    if (!sound->mute) {
        if (sound->floatAllow) {
            xthal_set_cpenable(true);
            xthal_save_cp0(cp0_regs);

            for (size_t i = 0; i < sound->outputsCount; i++) {
                tsgl_sound_setOutputValue(sound->outputs[i],
                    TSGL_MATH_MIN(convertPcm(sound, ptr + ((i % sound->channels) * sound->bit_rate)) * sound->volume, 255)
                );
            }

            xthal_restore_cp0(cp0_regs);
            xthal_set_cpenable(false);
        } else {
            for (size_t i = 0; i < sound->outputsCount; i++) {
                tsgl_sound_setOutputValue(sound->outputs[i],
                    convertPcm(sound, ptr + ((i % sound->channels) * sound->bit_rate))
                );
            }
        }
    }

    sound->position += sound->bit_rate * sound->channels;
    if (sound->position >= sound->len) {
        sound->position = 0;
        if (sound->loop) {
            //timer_group_enable_alarm_in_isr(sound->timerGroup, sound->timer);
            return true;
        } else if (sound->freeAfterPlay) {
            tsgl_sound_free(sound);
        } else {
            tsgl_sound_stop(sound);
        }
    } else {
        //timer_group_enable_alarm_in_isr(sound->timerGroup, sound->timer);
        return true;
    }

    return false;
}

static void _freeOutputs(tsgl_sound* sound) {
    if (sound->freeOutputs) {
        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_freeOutput(sound->outputs[i]);
        }
    }
    free(sound->outputs);
}

esp_err_t tsgl_sound_load_pcm(tsgl_sound* sound, size_t bufferSize, int64_t caps, const char* path, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_pcm_format pcm_format) {
    memset(sound, 0, sizeof(tsgl_sound));
    sound->file = fopen(path, "rb");
    if (sound->file == NULL) return ESP_FAIL;

    sound->speed = 1.0;
    sound->volume = 1.0;
    sound->len = tsgl_filesystem_size(path);
    sound->sample_rate = sample_rate;
    sound->bit_rate = bit_rate;
    sound->channels = channels;
    sound->pcm_format = pcm_format;
    sound->bufferSize = bufferSize;

    if (bufferSize != TSGL_SOUND_FULLBUFFER) {
        uint16_t t = bit_rate * channels;
        bufferSize = (bufferSize / t) * t;
        sound->bufferSize = bufferSize;

        sound->data = tsgl_malloc(bufferSize, caps);
        if (sound->data == NULL) {
            ESP_LOGE(TAG, "the first buffer for the sound could not be allocated: %i bytes", bufferSize);
            memset(sound, 0, sizeof(tsgl_sound));
            return ESP_ERR_NO_MEM;
        }

        sound->buffer = tsgl_malloc(bufferSize, caps);
        if (sound->buffer == NULL) {
            ESP_LOGE(TAG, "the second buffer for the sound could not be allocated: %i bytes", bufferSize);
            memset(sound, 0, sizeof(tsgl_sound));
            return ESP_ERR_NO_MEM;
        }

        sound->soundTask = true;
        xTaskCreate(_soundTask, NULL, 2048, sound, 1, &sound->task);
    } else {
        sound->bufferSize = sound->len;

        sound->data = tsgl_malloc(sound->len, caps);
        if (sound->data == NULL) {
            ESP_LOGE(TAG, "the full buffer for the sound could not be allocated: %i bytes", sound->len);
            memset(sound, 0, sizeof(tsgl_sound));
            return ESP_ERR_NO_MEM;
        }

        fread(sound->data, sound->bit_rate, sound->bufferSize, sound->file);
        fclose(sound->file);
        sound->file = NULL;
    }
    return ESP_OK;
}

esp_err_t tsgl_sound_instance(tsgl_sound* sound, tsgl_sound* parent) {
    if (parent->buffer != NULL) {
        ESP_LOGE(TAG, "it is not possible to create an instance of a track with dynamic loading");
        return ESP_FAIL;
    }

    memcpy(sound, parent, sizeof(tsgl_sound));
    sound->playing = false;
    return ESP_OK;
}

void tsgl_sound_setOutputs(tsgl_sound* sound, tsgl_sound_output** outputs, size_t outputsCount, bool freeOutputs) {
    _freeOutputs(sound);
    sound->outputsCount = outputsCount;
    sound->outputs = malloc(outputsCount * sizeof(size_t));
    for (size_t i = 0; i < sound->outputsCount; i++) {
        sound->outputs[i] = outputs[i];
    }
    sound->freeOutputs = freeOutputs;
}

void tsgl_sound_setSpeed(tsgl_sound* sound, float speed) {
    sound->speed = speed;
    if (sound->playing) {
        //ESP_ERROR_CHECK(timer_set_alarm_value(sound->timerGroup, sound->timer, APB_CLK_FREQ / 8 / sound->sample_rate / speed));
    }
}

void tsgl_sound_setPause(tsgl_sound* sound, bool pause) {
    if (sound->pause == pause) return;
    sound->pause = pause;
    if (pause) {
        ESP_ERROR_CHECK(gptimer_stop(sound->timer));
        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_setOutputValue(sound->outputs[i], 0);
        }
    } else {
        ESP_ERROR_CHECK(gptimer_start(sound->timer));
    }
}

void tsgl_sound_setLoop(tsgl_sound* sound, bool loop) {
    sound->loop = loop;
}

void tsgl_sound_setVolume(tsgl_sound* sound, float volume) {
    sound->volume = volume;
    if (volume == 1) {
        sound->floatAllow = false;
        sound->mute = false;
    } else if (volume == 0) {
        sound->floatAllow = false;
        sound->mute = true;
    } else {
        sound->floatAllow = true;
        sound->mute = false;
    }
}

void tsgl_sound_play(tsgl_sound* sound) {
    if (sound->playing) {
        ESP_LOGW(TAG, "tsgl_sound_play skipped. the track is already playing");
        return;
    } else if (sound->data == NULL) {
        ESP_LOGE(TAG, "tsgl_sound_play skipped. uninitialized audio cannot be started");
        return;
    }

    /*
    timer_config_t config = {
		.divider = 8,
		.counter_dir = TIMER_COUNT_UP,
		.counter_en = TIMER_PAUSE,
		.alarm_en = TIMER_ALARM_EN,
		.intr_type = TIMER_INTR_LEVEL,
		.auto_reload = 1
	};
    */

	//ESP_ERROR_CHECK(timer_init(sound->timerGroup, sound->timer, &config));
	//ESP_ERROR_CHECK(timer_set_counter_value(sound->timerGroup, sound->timer, 0x00000000ULL));
	//ESP_ERROR_CHECK(timer_set_alarm_value(sound->timerGroup, sound->timer, APB_CLK_FREQ / config.divider / sound->sample_rate / sound->speed));
	//ESP_ERROR_CHECK(timer_enable_intr(sound->timerGroup, sound->timer));
	//timer_isr_register(sound->timerGroup, sound->timer, _timer_ISR, sound, ESP_INTR_FLAG_IRAM, NULL);
	//timer_start(sound->timerGroup, sound->timer);

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0, // counter will reload with 0 on alarm event
        .alarm_count = 1000000, // period = 1s @resolution 1MHz
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };
    

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = sound->sample_rate * sound->speed, // 1MHz, 1 tick = 1us
    };
  
    gptimer_event_callbacks_t callback_config = {
        .on_alarm = _timer_ISR,
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &sound->timer));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(sound->timer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(sound->timer, &callback_config, sound));
    ESP_ERROR_CHECK(gptimer_enable(sound->timer));
    ESP_ERROR_CHECK(gptimer_start(sound->timer));

    sound->playing = true;
}

void tsgl_sound_stop(tsgl_sound* sound) {
    if (!sound->playing) {
        ESP_LOGW(TAG, "tsgl_sound_stop skipped. the track is not playing");
        return;
    }

    ESP_ERROR_CHECK(gptimer_stop(sound->timer));
    ESP_ERROR_CHECK(gptimer_disable(sound->timer));
    ESP_ERROR_CHECK(gptimer_del_timer(sound->timer));
    sound->playing = false;
}

void tsgl_sound_free(tsgl_sound* sound) {
    if (sound->playing) tsgl_sound_stop(sound);
    if (sound->soundTask) vTaskDelete(sound->task);
    if (sound->buffer != NULL) free(sound->buffer);
    if (sound->data != NULL) free(sound->data);
    if (sound->file != NULL) fclose(sound->file);
    _freeOutputs(sound);
    if (sound->heap) free(sound);
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

tsgl_sound_output* tsgl_sound_newLedcOutput(gpio_num_t pin) {
    tsgl_sound_output* output = calloc(1, sizeof(tsgl_sound_output));
    output->ledc = calloc(1, sizeof(tsgl_ledc));
    if (ESP_ERROR_CHECK_WITHOUT_ABORT(tsgl_ledc_newFast(output->ledc, pin, false, 0)) != ESP_OK) {
        free(output->ledc);
        output->ledc = NULL;
    }
    return output;
}

void IRAM_ATTR tsgl_sound_setOutputValue(tsgl_sound_output* output, uint8_t value) {
    #ifdef HARDWARE_DAC
        if (output->channel != NULL) {
            dac_oneshot_output_voltage(*output->channel, value);
        }
    #endif
    if (output->ledc != NULL) {
        tsgl_ledc_rawSet(output->ledc, value);
    }
}

void tsgl_sound_freeOutput(tsgl_sound_output* output) {
    #ifdef HARDWARE_DAC
        if (output->channel != NULL) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(dac_oneshot_del_channel(*output->channel));
            free(output->channel);
        }
    #endif
    if (output->ledc != NULL) {
        tsgl_ledc_free(output->ledc);
        free(output->ledc);
    }
    free(output);
}