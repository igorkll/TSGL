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

static atomic_flag global_sounds_lock = ATOMIC_FLAG_INIT;
static gptimer_handle_t global_timer;
static bool global_timer_enabled;
static int global_timer_freq = 0;

static tsgl_sound** global_sounds;
static size_t global_sounds_index = 0;
static size_t global_sounds_max_count = 0;

bool tsgl_sound_force_enable_output = false;

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

    vTaskSuspend(NULL);

    while (true) {
        void* buffer;
        if (sound->doubleSwapBuffer && !sound->readFromStart) {
            buffer = sound->buffer2;
        } else {
            buffer = sound->buffer;
        }

        if (sound->loop && sound->readFromStart) {
            //printf("seek start\n");
            fseek(sound->file, sound->offset, SEEK_SET);
        }
        
        //printf("read\n");
        size_t bytesRead = fread(buffer, 1, sound->bufferSize, sound->file);
        size_t setZeroSize = sound->bufferSize - bytesRead;
        if (setZeroSize > 0) memset((char*)buffer + bytesRead, 0, setZeroSize);

        if (sound->doubleSwapBuffer && sound->readFromStart) {
            bytesRead = fread(sound->buffer2, 1, sound->bufferSize, sound->file);
            size_t setZeroSize = sound->bufferSize - bytesRead;
            if (setZeroSize > 0) memset((char*)sound->buffer2 + bytesRead, 0, setZeroSize);
        }

        sound->readFromStart = false;
        sound->tempStop = false;
        if (sound->use_local_timer && sound->localTimerStopped) {
            gptimer_start(sound->timer);
            sound->localTimerStopped = false;
        }

        vTaskSuspend(NULL);
    }
}

static void _soundServiceTask(void* _sound) {
    tsgl_sound* sound = _sound;
    
    //vTaskSuspend(NULL);

    while (true) {
        if (sound->callback_end_run) {
            sound->callback_end_run = false;
            if (!sound->loop) tsgl_sound_stop(sound);
            if (sound->callback_end != NULL) sound->callback_end(sound);
            if (sound->freeOnEnd) {
                sound->task_service_used = false;
                if (sound->isInstance) {
                    tsgl_sound_free_instance(sound);
                } else {
                    tsgl_sound_free(sound);
                }
                vTaskDelete(NULL);
                return;
            }
        }

        vTaskSuspend(NULL);
    }
}

static void IRAM_ATTR _resetDfpwmState(tsgl_sound* sound) {
    float cutoff_mul = sound->cutoff_mul > 0 ? sound->cutoff_mul : 0.35;

    sound->bit_pos = 0;
    for (size_t i = 0; i < sound->channels; i++) {
        tsgl_dfpwm_reset(&sound->dfpwm_decode_state[i], sound->scaled_sample_rate, ((float)sound->scaled_sample_rate) * cutoff_mul);
    }
}

static void IRAM_ATTR _process_dfpwm(tsgl_sound* sound) {
    if (sound->dfpwm_decode_state) {
        void* ptr = sound->buffer + sound->bufferPosition;

        for (size_t i = 0; i < sound->channels; i++) {
            tsgl_dfpwm_decode(&sound->dfpwm_decode_state[i], (uint8_t*)ptr, sound->bit_pos + i);
        }
    }
}

static void _resetDfpwmDecoder(tsgl_sound* sound) {
    if (sound->dfpwm_decode_state == NULL) return;

    _resetDfpwmState(sound);
    _process_dfpwm(sound);
}

static void IRAM_ATTR _read_next_sample_raw(tsgl_sound* sound, int bufOffset) {
    bool readFile = false;

    sound->bufferPosition += bufOffset;
    if (sound->bufferPosition >= sound->bufferSize) {
        readFile = true;
    }

    sound->position += bufOffset;
    if (sound->position >= sound->len) {
        if (sound->loop) {
            _resetDfpwmState(sound);

            sound->position = 0;
            sound->readFromStart = true;

            if (sound->doubleSwapBuffer) {
                if (sound->use_local_timer) {
                    gptimer_stop(sound->timer);
                    sound->localTimerStopped = true;
                } else {
                    sound->tempStop = true;
                }
            }
        }
        readFile = sound->loop;

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
                sound->localTimerStopped = true;
            } else {
                sound->tempStop = true;
            }
            xTaskResumeFromISR(sound->task);
        }
    }
}

static void IRAM_ATTR _read_next_sample(tsgl_sound* sound) {
    if (sound->dfpwm_decode_state) {
        sound->bit_pos += sound->channels;
        if (sound->bit_pos >= 8) {
            sound->bit_pos = 0;
            _read_next_sample_raw(sound, 1);
        }

        _process_dfpwm(sound);
    } else {
        _read_next_sample_raw(sound, sound->bit_rate * sound->channels);
    }
}

static void IRAM_ATTR _addOutputsValues(tsgl_sound* sound) {
    void* ptr = sound->buffer + sound->bufferPosition;

    if (sound->dfpwm_decode_state) {
        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_output* output = sound->outputs[i];

            tsgl_sound_addOutputValue(output,
                (sound->dfpwm_decode_state[i % sound->channels].fq * sound->volume) / 255
            );
        }
    } else {
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
}

static bool IRAM_ATTR isSoundPlaying(tsgl_sound* sound) {
    return !sound->mute && sound->playing;
}

static bool IRAM_ATTR _global_timer_ISR(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx) {
    if (atomic_flag_test_and_set(&global_sounds_lock)) return false;

    for (size_t i = 0; i < global_sounds_index; i++) {
        tsgl_sound* sound = global_sounds[i];

        if (atomic_flag_test_and_set(&sound->lock)) continue;

        if (sound->playing) {
            if (isSoundPlaying(sound)) {
                _addOutputsValues(sound);
            }

            uint64_t acc = (uint64_t)sound->phase + sound->phase_step;
            while (acc >= (1ULL << 32)) {
                _read_next_sample(sound);
                sound->math_block_flag = true;
                acc -= (1ULL << 32);
                if (acc >= (1ULL << 32)) continue;
            }
            sound->phase = acc;
        }

        atomic_flag_clear(&sound->lock);
    }

    for (size_t i = 0; i < global_sounds_index; i++) {
        tsgl_sound* sound = global_sounds[i];

        if (atomic_flag_test_and_set(&sound->lock)) continue;

        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_output* output = sound->outputs[i];
            output->processed = true;
        }

        atomic_flag_clear(&sound->lock);
    }

    for (size_t i = 0; i < global_sounds_index; i++) {
        tsgl_sound* sound = global_sounds[i];

        if (atomic_flag_test_and_set(&sound->lock)) continue;

        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_output* output = sound->outputs[i];
            if (output->processed) {
                if (tsgl_sound_force_enable_output || output->count > 0) {
                    tsgl_sound_flushOutput(output);
                } else {
                    tsgl_sound_rawSetOutput(output, 0);
                }
                output->processed = false;
            }
        }

        atomic_flag_clear(&sound->lock);
    }

    atomic_flag_clear(&global_sounds_lock);
    return false;
}

static bool IRAM_ATTR _timer_ISR(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx) {
    tsgl_sound* sound = user_ctx;

    if (atomic_flag_test_and_set(&sound->lock)) return false;
    
    if (sound->callback_end_run) {
        atomic_flag_clear(&sound->lock);
        return false;
    }

    if (tsgl_sound_force_enable_output || isSoundPlaying(sound)) {
        _addOutputsValues(sound);

        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_output* output = sound->outputs[i];
            tsgl_sound_flushOutput(output);
        }
    } else {
        for (size_t i = 0; i < sound->outputsCount; i++) {
            tsgl_sound_output* output = sound->outputs[i];
            tsgl_sound_rawSetOutput(output, 0);
        }
    }

    _read_next_sample(sound);

    atomic_flag_clear(&sound->lock);

    return false;
}

static void _initTimer(tsgl_sound* sound) {
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1,
        .flags = {
            .auto_reload_on_alarm = true
        }
    };

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = sound->scaled_sample_rate // 1MHz, 1 tick = 1us
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
        if (!tsgl_sound_force_enable_output) tsgl_sound_rawSetOutput(output, 0);
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

    _resetDfpwmDecoder(sound);
}

void tsgl_sound_allocatePcmDecoder(tsgl_sound* sound, float cutoff_mul) {
    sound->dfpwm_decode_state = malloc(sound->channels * sizeof(tsgl_dfpwm_decode_state));
    sound->cutoff_mul = cutoff_mul;
    _resetDfpwmDecoder(sound);
}

void tsgl_sound_enableGlobalTimer(int freq, size_t max_sounds) {
    if (use_global_timer) return;

    while (atomic_flag_test_and_set(&global_sounds_lock));

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

    atomic_flag_clear(&global_sounds_lock);
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
    sound->scaled_sample_rate = sound->sample_rate * sound->speed;

    if (!sound->use_local_timer) {
        //sound->global_timer_div = (global_timer_freq / sound->scaled_sample_rate) - 1;

        uint64_t step = ((uint64_t)sound->scaled_sample_rate << 32) / (uint64_t)global_timer_freq;
        sound->phase_step = step;
    }
}

esp_err_t tsgl_sound_load_pcmPartEx(tsgl_sound* sound, size_t offset, size_t loadsize, size_t bufferSize, int64_t caps, const char* path, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_pcm_format pcm_format, bool doubleSwapBuffer) {
    memset(sound, 0, sizeof(tsgl_sound));
    sound->inited = true;
    sound->math_block_flag = true;

    atomic_flag_clear(&sound->lock);

    sound->file = tsgl_filesystem_open(path, "rb");
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
            fclose(sound->file);
            ESP_LOGE(TAG, "the buffer for the sound could not be allocated: %i bytes", bufferSize);
            memset(sound, 0, sizeof(tsgl_sound));
            return ESP_ERR_NO_MEM;
        }

        fread(sound->buffer, sound->bit_rate, bufferSize, sound->file);

        if (doubleSwapBuffer) {
            sound->buffer2 = tsgl_malloc(bufferSize, caps);
            if (sound->buffer2 == NULL) {
                fclose(sound->file);
                free(sound->buffer);
                ESP_LOGE(TAG, "the buffer2 for the sound could not be allocated: %i bytes", sound->len);
                memset(sound, 0, sizeof(tsgl_sound));
                return ESP_ERR_NO_MEM;
            }

            fread(sound->buffer2, sound->bit_rate, bufferSize, sound->file);
        }

        xTaskCreate(_soundTask, NULL, TSGL_SOUND_STACK_SIZE, sound, 1, &sound->task);
        sound->task_used = true;
    } else {
        sound->bufferSize = sound->len;

        sound->buffer = tsgl_malloc(sound->len, caps);
        if (sound->buffer == NULL) {
            fclose(sound->file);
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
    if (parent->task_used) {
        ESP_LOGE(TAG, "it is not possible to create an instance of a track with dynamic loading");
        return ESP_FAIL;
    }

    memcpy(sound, parent, sizeof(tsgl_sound));

    // ------------------- recreate

    sound->isInstance = true;
    sound->playing = false;
    sound->task_service = NULL;

    return ESP_OK;
}

void tsgl_sound_setOutputs(tsgl_sound* sound, tsgl_sound_output** outputs, size_t outputsCount, bool freeOutputs) {
    while (atomic_flag_test_and_set(&global_sounds_lock));
    while (atomic_flag_test_and_set(&sound->lock));

    _freeOutputs(sound);

    sound->outputsCount = outputsCount;
    sound->outputs = malloc(outputsCount * sizeof(size_t));
    for (size_t i = 0; i < sound->outputsCount; i++) {
        tsgl_sound_output* output = outputs[i];
        sound->outputs[i] = output;
    }
    sound->freeOutputs = freeOutputs;

    atomic_flag_clear(&sound->lock);
    atomic_flag_clear(&global_sounds_lock);
}

void tsgl_sound_setOutputsRaw(tsgl_sound* sound, tsgl_sound_output** outputs, size_t outputsCount) {
    while (atomic_flag_test_and_set(&global_sounds_lock));
    while (atomic_flag_test_and_set(&sound->lock));

    sound->outputsCount = outputsCount;
    sound->outputs = malloc(outputsCount * sizeof(size_t));
    for (size_t i = 0; i < sound->outputsCount; i++) {
        tsgl_sound_output* output = outputs[i];
        sound->outputs[i] = output;
    }

    atomic_flag_clear(&sound->lock);
    atomic_flag_clear(&global_sounds_lock);
}

void tsgl_sound_setSpeed(tsgl_sound* sound, float speed) {
    while (atomic_flag_test_and_set(&sound->lock));

    sound->speed = speed;
    afterUpdateSpeed(sound);
    _resetDfpwmDecoder(sound);

    if (sound->playing && sound->use_local_timer) {
        gptimer_stop(sound->timer);
        gptimer_disable(sound->timer);
        gptimer_del_timer(sound->timer);
        _initTimer(sound);
        ESP_ERROR_CHECK_WITHOUT_ABORT(gptimer_start(sound->timer));
    }

    atomic_flag_clear(&sound->lock);
}

void tsgl_sound_setLoop(tsgl_sound* sound, bool loop) {
    while (atomic_flag_test_and_set(&sound->lock));
    sound->loop = loop;
    atomic_flag_clear(&sound->lock);
}

void tsgl_sound_setVolume(tsgl_sound* sound, float volume) {
    while (atomic_flag_test_and_set(&sound->lock));
    if (volume == 0) {
        sound->volume = 0;
        sound->mute = true;
    } else {
        sound->volume = volume * 255;
        sound->mute = false;
    }
    atomic_flag_clear(&sound->lock);
}

void tsgl_sound_setPosition(tsgl_sound* sound, size_t position) {
    while (atomic_flag_test_and_set(&sound->lock));
    bool timerAction = sound->playing && sound->use_local_timer;
    if (timerAction) gptimer_stop(sound->timer);
    _setPosition(sound, position);
    if (timerAction) gptimer_start(sound->timer);
    atomic_flag_clear(&sound->lock);
}

void tsgl_sound_seek(tsgl_sound* sound, int offset) {
    while (atomic_flag_test_and_set(&sound->lock));
    bool timerAction = sound->playing && sound->use_local_timer;
    if (timerAction) gptimer_stop(sound->timer);
    int64_t newpos = ((int64_t)sound->position) + offset;
    if (newpos < 0) newpos = 0;
    _setPosition(sound, newpos);
    if (timerAction) gptimer_start(sound->timer);
    atomic_flag_clear(&sound->lock);
}

void tsgl_sound_play(tsgl_sound* sound) {
    if (sound->playing) {
        return;
    } else if (sound->buffer == NULL) {
        ESP_LOGE(TAG, "tsgl_sound_play skipped. uninitialized audio cannot be started");
        return;
    }

    if (!sound->first_start) {
        sound->first_start = true;

        xTaskCreate(_soundServiceTask, NULL, TSGL_SOUND_STACK_SIZE, sound, 1, &sound->task_service);
        sound->task_service_used = true;
        
        if (use_global_timer && global_sounds_index < global_sounds_max_count) {
            while (atomic_flag_test_and_set(&global_sounds_lock));
            global_sounds[global_sounds_index++] = sound;
            atomic_flag_clear(&global_sounds_lock);
        }
    }

    while (atomic_flag_test_and_set(&sound->lock));
    sound->playing = true;
    if (sound->use_local_timer) {
        _initTimer(sound);
        gptimer_start(sound->timer);
    } else if (!global_timer_enabled) {
        gptimer_start(global_timer);
        global_timer_enabled = true;
    }
    atomic_flag_clear(&sound->lock);
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
        while (atomic_flag_test_and_set(&global_sounds_lock));
        for (size_t i = 0; i < global_sounds_index; i++) {
            if (global_sounds[i]->playing) {
                found_playing = true;
                break;
            }
        }
        if (!found_playing && global_timer_enabled) {
            _resetOutputs(sound);
            gptimer_stop(global_timer);
            global_timer_enabled = false;
        }
        atomic_flag_clear(&global_sounds_lock);
    }
}

void tsgl_sound_stop(tsgl_sound* sound) {
    if (!sound->playing) return;
    while (atomic_flag_test_and_set(&sound->lock));
    _stop(sound);
    atomic_flag_clear(&sound->lock);
}

void tsgl_sound_free(tsgl_sound* sound) {
    while (atomic_flag_test_and_set(&sound->lock));
    if (sound->playing) _stop(sound);
    if (sound->task_used) {
        vTaskDelete(sound->task);
    }
    if (sound->task_service_used) {
        vTaskDelete(sound->task_service);
    }
    if (sound->buffer != NULL) free(sound->buffer);
    if (sound->buffer2 != NULL) free(sound->buffer2);
    if (sound->dfpwm_decode_state != NULL) free(sound->dfpwm_decode_state);
    if (use_global_timer) {
        while (atomic_flag_test_and_set(&global_sounds_lock));
        for (size_t i = 0; i < global_sounds_index; i++) {
            tsgl_sound* globalSound = global_sounds[i];
            if (globalSound == sound) {
                global_sounds[i] = global_sounds[global_sounds_index - 1];
                global_sounds_index--;
                break;
            }
        }
        atomic_flag_clear(&global_sounds_lock);
    }
    _freeOutputs(sound);
    atomic_flag_clear(&sound->lock);

    if (sound->file != NULL) {
        fclose(sound->file);
    }
    
    memset(sound, 0, sizeof(tsgl_sound));
    if (sound->heap) free(sound);
}

void tsgl_sound_free_instance(tsgl_sound* sound) {
    while (atomic_flag_test_and_set(&sound->lock));
    if (sound->playing) _stop(sound);
    if (sound->task_used) {
        vTaskDelete(sound->task);
    }
    if (sound->task_service_used) {
        vTaskDelete(sound->task_service);
    }
    if (sound->dfpwm_decode_state != NULL) free(sound->dfpwm_decode_state);
    if (use_global_timer) {
        while (atomic_flag_test_and_set(&global_sounds_lock));
        for (size_t i = 0; i < global_sounds_index; i++) {
            tsgl_sound* globalSound = global_sounds[i];
            if (globalSound == sound) {
                global_sounds[i] = global_sounds[global_sounds_index - 1];
                global_sounds_index--;
                break;
            }
        }
        atomic_flag_clear(&global_sounds_lock);
    }
    _freeOutputs(sound);
    atomic_flag_clear(&sound->lock);

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
