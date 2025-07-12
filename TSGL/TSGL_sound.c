#include "TSGL_sound.h"
#include "TSGL_filesystem.h"
#include "TSGL_math.h"
#include <esp_attr.h>
#include <esp_heap_caps.h>
#include <soc/soc.h>
#include <esp_log.h>
#include <string.h>
#include <limits.h>

static const char* TAG = "TSGL_sound";

static int IRAM_ATTR _convertPcm(tsgl_sound* sound, void* source) {
    if (sound->bit_rate == 4) {
        switch (sound->pcm_format) {
            case tsgl_sound_pcm_unsigned:
                return *((uint32_t*)source) - 2147483648.0;
            
            case tsgl_sound_pcm_signed:
                return *((int32_t*)source);
        }
    } else if (sound->bit_rate == 2) {
        switch (sound->pcm_format) {
            case tsgl_sound_pcm_unsigned:
                return *((uint16_t*)source) - 32768.0;
            
            case tsgl_sound_pcm_signed:
                return *((int16_t*)source);
        }
    } else {
        switch (sound->pcm_format) {
            case tsgl_sound_pcm_unsigned:
                return *((uint8_t*)source) - 128;
            
            case tsgl_sound_pcm_signed:
                return *((int8_t*)source);
        }
    }
    return 0;
}

static void _soundTask(void* _sound) {
    tsgl_sound* sound = _sound;
    fread(sound->buffer, sound->bit_rate, sound->bufferSize, sound->file);
    vTaskSuspend(NULL);

    while (true) {
        if (sound->reload) {
            sound->reload = false;
            fseek(sound->file, sound->position + sound->offset, SEEK_SET);
        }
        fread(sound->buffer, sound->bit_rate, sound->bufferSize, sound->file);
        gptimer_start(sound->timer);

        vTaskDelay(1);
        vTaskSuspend(NULL);
    }
}

static bool IRAM_ATTR _timer_ISR(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx) {
    tsgl_sound* sound = user_ctx;

    if (!sound->mute) {
        void* ptr = sound->buffer + sound->bufferPosition;
        int div;
        if (sound->bit_rate == 4) {
            div = 256 * 256 * 256;
        } else if (sound->bit_rate == 2) {
            div = 256;
        } else {
            div = 1;
        }

        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_addOutputValue(sound->outputs[i],
                (_convertPcm(sound, ptr + ((i % sound->channels) * sound->bit_rate)) * sound->volume) / 255 / div
            );
            tsgl_sound_flushOutput(sound);
        }
    }

    int bufOffset = sound->bit_rate * sound->channels;
    bool readFile = false;

    sound->bufferPosition += bufOffset;
    if (sound->bufferPosition >= sound->bufferSize) {
        readFile = true;
    }

    sound->position += bufOffset;
    if (sound->position >= sound->len) {
        sound->position = 0;
        if (sound->loop) {
            sound->reload = true;
            readFile = true;
        } else {
            tsgl_sound_stop(sound);
        }
    }

    if (readFile) {
        sound->bufferPosition = 0;
        if (sound->file != NULL) {
            gptimer_stop(sound->timer);
            xTaskResumeFromISR(sound->task);
        }
    }

    return false;
}

static void _initTimer(tsgl_sound* sound) {
    uint64_t freq = sound->sample_rate * sound->speed;

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1,
        .flags = {
            .auto_reload_on_alarm = true
        }
    };

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = freq // 1MHz, 1 tick = 1us
    };
  
    gptimer_event_callbacks_t callback_config = {
        .on_alarm = _timer_ISR,
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &sound->timer));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(sound->timer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(sound->timer, &callback_config, sound));
    ESP_ERROR_CHECK(gptimer_enable(sound->timer));
}

static void _freeOutputs(tsgl_sound* sound) {
    if (sound->freeOutputs) {
        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_freeOutput(sound->outputs[i]);
        }
    }
    free(sound->outputs);
}

static void _setPosition(tsgl_sound* sound, size_t position) {
    sound->position = position;
    //if (sound->position < 0) sound->position = 0;
    if (sound->position >= sound->len) sound->position = sound->len - 1;

    if (sound->file != NULL) {
        sound->bufferPosition = 0;
        fseek(sound->file, sound->position + sound->offset, SEEK_SET);
        fread(sound->buffer, sound->bit_rate, sound->bufferSize, sound->file);
    } else {
        sound->bufferPosition = sound->position;
    }
}


esp_err_t tsgl_sound_load_pcm(tsgl_sound* sound, size_t bufferSize, int64_t caps, const char* path, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_pcm_format pcm_format) {
    return tsgl_sound_load_pcmPart(sound, 0, 0, bufferSize, caps, path, sample_rate, bit_rate, channels, pcm_format);
}

esp_err_t tsgl_sound_load_pcmPart(tsgl_sound* sound, size_t offset, size_t loadsize, size_t bufferSize, int64_t caps, const char* path, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_pcm_format pcm_format) {
    memset(sound, 0, sizeof(tsgl_sound));
    sound->file = fopen(path, "rb");
    if (sound->file == NULL) return ESP_FAIL;
    fseek(sound->file, offset, SEEK_SET);

    sound->offset = offset;
    sound->speed = 1.0;
    tsgl_sound_setVolume(sound, 1);
    if (loadsize == 0) {
        sound->len = tsgl_filesystem_size(path) - offset;
    } else {
        sound->len = loadsize;
    }
    sound->sample_rate = sample_rate;
    sound->bit_rate = bit_rate;
    sound->channels = channels;
    sound->pcm_format = pcm_format;
    sound->bufferSize = bufferSize;

    if (bufferSize != TSGL_SOUND_FULLBUFFER) {
        uint16_t t = bit_rate * channels;
        bufferSize = (bufferSize / t) * t;
        sound->bufferSize = bufferSize;

        sound->buffer = tsgl_malloc(bufferSize, caps);
        if (sound->buffer == NULL) {
            ESP_LOGE(TAG, "the buffer for the sound could not be allocated: %i bytes", bufferSize);
            memset(sound, 0, sizeof(tsgl_sound));
            return ESP_ERR_NO_MEM;
        }

        xTaskCreate(_soundTask, NULL, 2048, sound, 1, &sound->task);
    } else {
        sound->bufferSize = sound->len;

        sound->buffer = tsgl_malloc(sound->len, caps);
        if (sound->buffer == NULL) {
            ESP_LOGE(TAG, "the full buffer for the sound could not be allocated: %i bytes", sound->len);
            memset(sound, 0, sizeof(tsgl_sound));
            return ESP_ERR_NO_MEM;
        }

        fread(sound->buffer, sound->bit_rate, sound->bufferSize, sound->file);
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
        gptimer_stop(sound->timer);
        gptimer_disable(sound->timer);
        gptimer_del_timer(sound->timer);
        _initTimer(sound);
        ESP_ERROR_CHECK_WITHOUT_ABORT(gptimer_start(sound->timer));
    }
}

void tsgl_sound_setLoop(tsgl_sound* sound, bool loop) {
    sound->loop = loop;
}

void tsgl_sound_setVolume(tsgl_sound* sound, float volume) {
    if (volume == 0) {
        sound->volume = 0;
        sound->mute = true;
    } else {
        sound->volume = volume * 255;
        sound->mute = false;
    }
}

void tsgl_sound_setPosition(tsgl_sound* sound, size_t position) {
    bool timerAction = sound->playing;
    if (timerAction) gptimer_stop(sound->timer);
    _setPosition(sound, position);
    if (timerAction) gptimer_start(sound->timer);
}

void tsgl_sound_seek(tsgl_sound* sound, int offset) {
    bool timerAction = sound->playing;
    if (timerAction) gptimer_stop(sound->timer);
    int64_t newpos = ((int64_t)sound->position) + offset;
    if (newpos < 0) newpos = 0;
    _setPosition(sound, newpos);
    if (timerAction) gptimer_start(sound->timer);
}

void tsgl_sound_play(tsgl_sound* sound) {
    if (sound->playing) {
        ESP_LOGW(TAG, "tsgl_sound_play skipped. the track is already playing");
        return;
    } else if (sound->buffer == NULL) {
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

    _initTimer(sound);
    gptimer_start(sound->timer);
    sound->playing = true;
}

void tsgl_sound_stop(tsgl_sound* sound) {
    if (!sound->playing) {
        ESP_LOGW(TAG, "tsgl_sound_stop skipped. the track is not playing");
        return;
    }

    gptimer_stop(sound->timer);
    gptimer_disable(sound->timer);
    gptimer_del_timer(sound->timer);
    sound->playing = false;
}

void tsgl_sound_free(tsgl_sound* sound) {
    if (sound->playing) tsgl_sound_stop(sound);
    if (sound->buffer != NULL) free(sound->buffer);
    if (sound->file != NULL) {
        vTaskDelete(sound->task);
        fclose(sound->file);
    }
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

void IRAM_ATTR tsgl_sound_addOutputValue(tsgl_sound_output* output, int value) {
    output->value += value;
}

void IRAM_ATTR tsgl_sound_flushOutput(tsgl_sound_output* output) {
    if (output == NULL) return;

    uint8_t value = TSGL_MATH_CLAMP(output->value + 128, 0, 255);
    
    #ifdef HARDWARE_DAC
        if (output->channel != NULL) {
            dac_oneshot_output_voltage(*output->channel, value);
        }
    #endif
    
    if (output->ledc != NULL) {
        tsgl_ledc_rawSet(output->ledc, value);
    }

    output->value = 0;
}

void tsgl_sound_freeOutput(tsgl_sound_output* output) {
    output->value = 0;
    tsgl_sound_flushOutput(output);

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