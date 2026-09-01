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

static bool use_global_timer = false;

static portMUX_TYPE global_sounds_lock = portMUX_INITIALIZER_UNLOCKED;
static gptimer_handle_t global_timer;
static int global_timer_freq = 0;

static tsgl_sound** global_sounds;
static size_t global_sounds_index = 0;
static size_t global_sounds_max_count = 0;

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
    
    void* buffer;
    if (sound->doubleSwapBuffer) {
        buffer = sound->buffer2;
    } else {
        buffer = sound->buffer;
    }

    vTaskSuspend(NULL);

    while (true) {
        if (sound->doubleSwapBuffer) {
            buffer = sound->buffer2;
        } else {
            buffer = sound->buffer;
        }
        
        if (sound->reload) {
            sound->reload = false;
            fseek(sound->file, sound->position + sound->offset, SEEK_SET);
        }
        fread(buffer, sound->bit_rate, sound->bufferSize, sound->file);
        if (!sound->doubleSwapBuffer) {
            if (sound->use_local_timer) {
                gptimer_start(sound->timer);
            }
        }

        vTaskSuspend(NULL);
    }
}

static void _soundServiceTask(void* _sound) {
    tsgl_sound* sound = _sound;
    
    vTaskSuspend(NULL);

    while (true) {
        if (sound->callback_end_run) {
            sound->callback_end_run = false;
            if (!sound->loop) tsgl_sound_stop(sound);
            if (sound->callback_end != NULL) sound->callback_end(sound);
            if (sound->freeOnEnd) {
                sound->task_service_used = false;
                tsgl_sound_free(sound);
                vTaskDelete(NULL);
                return;
            }
        }

        vTaskSuspend(NULL);
    }
}

static void IRAM_ATTR _read_next_block(tsgl_sound* sound, int bufOffset) {
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
        }

        sound->callback_end_run = true;
        xTaskResumeFromISR(sound->task_service);
    }

    if (readFile) {
        sound->bufferPosition = 0;
        if (sound->task_used) {
            if (sound->doubleSwapBuffer) {
                void* buffer = sound->buffer;
                sound->buffer = sound->buffer2;
                sound->buffer2 = buffer;
            } else if (sound->use_local_timer) {
                gptimer_stop(sound->timer);
            }
            xTaskResumeFromISR(sound->task);
        }
    }
}

static bool IRAM_ATTR _timer_ISR(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx) {
    tsgl_sound* sound = user_ctx;

    portENTER_CRITICAL_ISR(&sound->lock);
    
    if (sound->callback_end_run) {
        portEXIT_CRITICAL_ISR(&sound->lock);
        return false;
    }

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
            tsgl_sound_output* output = sound->outputs[i];

            tsgl_sound_addOutputValue(output,
                (_convertPcm(sound, ptr + ((i % sound->channels) * sound->bit_rate)) * sound->volume) / 255 / div
            );

            tsgl_sound_flushOutput(output);
        }
    } else {
        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_output* output = sound->outputs[i];
            tsgl_sound_rawSetOutput(output, 0);
        }
    }

    _read_next_block(sound, sound->bit_rate * sound->channels);

    portEXIT_CRITICAL_ISR(&sound->lock);

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

static void _resetOutputs(tsgl_sound* sound) {
    for (size_t i = 0; i < sound->outputsCount; i++) {
        tsgl_sound_output* output = sound->outputs[i];
        output->value = 0;
        tsgl_sound_rawSetOutput(output, 0);
    }
}

static void _freeOutputs(tsgl_sound* sound) {
    if (sound->freeOutputs) {
        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_freeOutput(sound->outputs[i]);
        }
    } else {
        _resetOutputs(sound);
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

static bool IRAM_ATTR _global_timer_ISR(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx) {
    portENTER_CRITICAL_ISR(&global_sounds_lock);
    for (size_t i = 0; i < global_sounds_index; i++) {
        tsgl_sound* sound = global_sounds[i];

        portENTER_CRITICAL_ISR(&sound->lock);

        if (sound->playing && !sound->callback_end_run) {
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
                    tsgl_sound_output* output = sound->outputs[i];

                    tsgl_sound_addOutputValue(output,
                        (_convertPcm(sound, ptr + ((i % sound->channels) * sound->bit_rate)) * sound->volume) / 255 / div
                    );
                }
            }

            if (sound->global_timer_state >= sound->global_timer_div) {
                _read_next_block(sound, sound->bit_rate * sound->channels);
                sound->global_timer_state = 0;
            } else {
                sound->global_timer_state++;
            }
        }

        portEXIT_CRITICAL_ISR(&sound->lock);
    }

    for (size_t i = 0; i < global_sounds_index; i++) {
        tsgl_sound* sound = global_sounds[i];

        portENTER_CRITICAL_ISR(&sound->lock);

        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_output* output = sound->outputs[i];
            output->processed = true;
        }

        portEXIT_CRITICAL_ISR(&sound->lock);
    }

    for (size_t i = 0; i < global_sounds_index; i++) {
        tsgl_sound* sound = global_sounds[i];

        portENTER_CRITICAL_ISR(&sound->lock);

        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_output* output = sound->outputs[i];
            if (output->processed) {
                if (output->count > 0) {
                    tsgl_sound_flushOutput(output);
                } else {
                    tsgl_sound_rawSetOutput(output, 0);
                }
                output->processed = false;
            }
        }

        portEXIT_CRITICAL_ISR(&sound->lock);
    }

    portEXIT_CRITICAL_ISR(&global_sounds_lock);
    return false;
}

void tsgl_sound_enableGlobalTimer(int freq, size_t max_sounds) {
    if (use_global_timer) return;

    portENTER_CRITICAL_ISR(&global_sounds_lock);

    global_sounds = calloc(max_sounds, sizeof(size_t));
    global_sounds_max_count = max_sounds;

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1,
        .flags = {
            .auto_reload_on_alarm = true
        }
    };

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = freq
    };
  
    gptimer_event_callbacks_t callback_config = {
        .on_alarm = _global_timer_ISR,
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &global_timer));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(global_timer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(global_timer, &callback_config, NULL));
    ESP_ERROR_CHECK(gptimer_enable(global_timer));

    global_timer_freq = freq;
    use_global_timer = true;

    portEXIT_CRITICAL_ISR(&global_sounds_lock);
}

esp_err_t tsgl_sound_load_pcm(tsgl_sound* sound, size_t bufferSize, int64_t caps, const char* path, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_pcm_format pcm_format) {
    return tsgl_sound_load_pcmPart(sound, 0, 0, bufferSize, caps, path, sample_rate, bit_rate, channels, pcm_format);
}

esp_err_t tsgl_sound_load_pcmEx(tsgl_sound* sound, size_t bufferSize, int64_t caps, const char* path, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_pcm_format pcm_format, bool doubleSwapBuffer) {
    return tsgl_sound_load_pcmPartEx(sound, 0, 0, bufferSize, caps, path, sample_rate, bit_rate, channels, pcm_format, doubleSwapBuffer);
}

esp_err_t tsgl_sound_load_pcmPart(tsgl_sound* sound, size_t offset, size_t loadsize, size_t bufferSize, int64_t caps, const char* path, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_pcm_format pcm_format) {
    return tsgl_sound_load_pcmPartEx(sound, offset, loadsize, bufferSize, caps, path, sample_rate, bit_rate, channels, pcm_format, false);
}

static void afterUpdateSpeed(tsgl_sound* sound) {
    if (!sound->use_local_timer) {
        sound->global_timer_div = (global_timer_freq / (sound->sample_rate * sound->speed)) - 1;
    }
}

esp_err_t tsgl_sound_load_pcmPartEx(tsgl_sound* sound, size_t offset, size_t loadsize, size_t bufferSize, int64_t caps, const char* path, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_pcm_format pcm_format, bool doubleSwapBuffer) {
    memset(sound, 0, sizeof(tsgl_sound));
    sound->lock = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;

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
    sound->doubleSwapBuffer = doubleSwapBuffer;
    sound->use_local_timer = !use_global_timer;

    afterUpdateSpeed(sound);

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

        fread(sound->buffer, sound->bit_rate, bufferSize, sound->file);

        if (doubleSwapBuffer) {
            sound->buffer2 = tsgl_malloc(bufferSize, caps);
            if (sound->buffer2 == NULL) {
                free(sound->buffer);
                ESP_LOGE(TAG, "the buffer2 for the sound could not be allocated: %i bytes", sound->len);
                memset(sound, 0, sizeof(tsgl_sound));
                return ESP_ERR_NO_MEM;
            }

            fread(sound->buffer2, sound->bit_rate, bufferSize, sound->file);
        }

        xTaskCreate(_soundTask, NULL, 1024 * 8, sound, 1, &sound->task);
        sound->task_used = true;
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

    xTaskCreate(_soundServiceTask, NULL, 1024 * 8, sound, 1, &sound->task_service);
    sound->task_service_used = true;

    if (use_global_timer && global_sounds_index < global_sounds_max_count) {
        portENTER_CRITICAL(&global_sounds_lock);
        global_sounds[global_sounds_index++] = sound;
        portEXIT_CRITICAL(&global_sounds_lock);
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
    portENTER_CRITICAL(&sound->lock);

    _freeOutputs(sound);

    sound->outputsCount = outputsCount;
    sound->outputs = malloc(outputsCount * sizeof(size_t));
    for (size_t i = 0; i < sound->outputsCount; i++) {
        tsgl_sound_output* output = outputs[i];
        sound->outputs[i] = output;
    }
    sound->freeOutputs = freeOutputs;

    portEXIT_CRITICAL(&sound->lock);
}

void tsgl_sound_setSpeed(tsgl_sound* sound, float speed) {
    portENTER_CRITICAL(&sound->lock);

    sound->speed = speed;

    afterUpdateSpeed(sound);

    if (sound->playing && sound->use_local_timer) {
        gptimer_stop(sound->timer);
        gptimer_disable(sound->timer);
        gptimer_del_timer(sound->timer);
        _initTimer(sound);
        ESP_ERROR_CHECK_WITHOUT_ABORT(gptimer_start(sound->timer));
    }

    portEXIT_CRITICAL(&sound->lock);
}

void tsgl_sound_setLoop(tsgl_sound* sound, bool loop) {
    portENTER_CRITICAL(&sound->lock);
    sound->loop = loop;
    portEXIT_CRITICAL(&sound->lock);
}

void tsgl_sound_setVolume(tsgl_sound* sound, float volume) {
    portENTER_CRITICAL(&sound->lock);
    if (volume == 0) {
        sound->volume = 0;
        sound->mute = true;
    } else {
        sound->volume = volume * 255;
        sound->mute = false;
    }
    portEXIT_CRITICAL(&sound->lock);
}

void tsgl_sound_setPosition(tsgl_sound* sound, size_t position) {
    portENTER_CRITICAL(&sound->lock);
    bool timerAction = sound->playing && sound->use_local_timer;
    if (timerAction) gptimer_stop(sound->timer);
    _setPosition(sound, position);
    if (timerAction) gptimer_start(sound->timer);
    portEXIT_CRITICAL(&sound->lock);
}

void tsgl_sound_seek(tsgl_sound* sound, int offset) {
    portENTER_CRITICAL(&sound->lock);
    bool timerAction = sound->playing && sound->use_local_timer;
    if (timerAction) gptimer_stop(sound->timer);
    int64_t newpos = ((int64_t)sound->position) + offset;
    if (newpos < 0) newpos = 0;
    _setPosition(sound, newpos);
    if (timerAction) gptimer_start(sound->timer);
    portEXIT_CRITICAL(&sound->lock);
}

void tsgl_sound_play(tsgl_sound* sound) {
    if (sound->playing) {
        return;
    } else if (sound->buffer == NULL) {
        ESP_LOGE(TAG, "tsgl_sound_play skipped. uninitialized audio cannot be started");
        return;
    }

    portENTER_CRITICAL(&sound->lock);
    sound->playing = true;
    if (sound->use_local_timer) {
        _initTimer(sound);
        gptimer_start(sound->timer);
    } else {
        gptimer_start(global_timer);
    }
    portEXIT_CRITICAL(&sound->lock);
}

static void _stop(tsgl_sound* sound) {
    sound->playing = false;

    if (sound->use_local_timer) {
        gptimer_stop(sound->timer);
        gptimer_disable(sound->timer);
        gptimer_del_timer(sound->timer);
        
        _resetOutputs(sound);
    } else {
        bool found_playing = false;
        portENTER_CRITICAL(&global_sounds_lock);
        for (size_t i = 0; i < global_sounds_index; i++) {
            if (global_sounds[i]->playing) {
                found_playing = true;
                break;
            }
        }
        if (!found_playing) {
            gptimer_stop(global_timer);
        }
        portEXIT_CRITICAL(&global_sounds_lock);
    }
}

void tsgl_sound_stop(tsgl_sound* sound) {
    if (!sound->playing) return;
    portENTER_CRITICAL(&sound->lock);
    _stop(sound);
    portEXIT_CRITICAL(&sound->lock);
}

void tsgl_sound_free(tsgl_sound* sound) {
    portENTER_CRITICAL(&sound->lock);
    if (sound->playing) _stop(sound);
    if (sound->task_used) {
        vTaskDelete(sound->task);
    }
    if (sound->task_service_used) {
        vTaskDelete(sound->task_service);
    }
    if (sound->buffer != NULL) free(sound->buffer);
    if (sound->buffer2 != NULL) free(sound->buffer2);
    _freeOutputs(sound);
    if (use_global_timer) {
        portENTER_CRITICAL(&global_sounds_lock);
        for (size_t i = 0; i < global_sounds_index; i++) {
            tsgl_sound* globalSound = global_sounds[i];
            if (globalSound == sound) {
                global_sounds[i] = global_sounds[global_sounds_index - 1];
                global_sounds_index--;
                break;
            }
        }
        portEXIT_CRITICAL(&global_sounds_lock);
    }
    portEXIT_CRITICAL(&sound->lock);

    if (sound->file != NULL) {
        fclose(sound->file);
    }
    
    memset(sound, 0, sizeof(tsgl_sound));
    if (sound->heap) free(sound);
}

void tsgl_sound_enableFreeOnEnd(tsgl_sound* sound, bool freeOnEnd) {
    sound->freeOnEnd = freeOnEnd;
}

void tsgl_sound_attachCallback_end(tsgl_sound* sound, void(*callback)(tsgl_sound* sound)) {
    sound->callback_end = callback;
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
    output->count++;
}

void IRAM_ATTR tsgl_sound_rawSetOutput(tsgl_sound_output* output, uint8_t value) {
    #ifdef HARDWARE_DAC
        if (output->channel != NULL) {
            dac_oneshot_output_voltage(*output->channel, value);
        }
    #endif

    if (output->ledc != NULL) {
        tsgl_ledc_rawSet(output->ledc, value);
    }
}

void IRAM_ATTR tsgl_sound_flushOutput(tsgl_sound_output* output) {
    if (output == NULL) return;

    uint8_t value = TSGL_MATH_CLAMP(output->value + 128, 0, 255);
    tsgl_sound_rawSetOutput(output, value);

    output->value = 0;
    output->count = 0;
}

void tsgl_sound_freeOutput(tsgl_sound_output* output) {
    output->value = 0;
    tsgl_sound_rawSetOutput(output, 0);

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
