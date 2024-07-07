#if __has_include(<driver/dac.h>)
#include "TSGL_sound.h"
#include "TSGL_filesystem.h"
#include <driver/timer.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "TSGL_sound";

static void _playerTask(void* _sound) {
    tsgl_sound* sound = _sound;
    dac_output_voltage(DAC_CHAN_0, *(audio->data + wav_pos));
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
        ESP_LOGE(TAG, "tsgl_sound_play skipped. the track is already playing");
        return;
    }

    sound->playing = true;
    sound->channel = channel;

}

void tsgl_sound_stop(tsgl_sound* sound) {
    sound->playing = false;
}

void tsgl_sound_free(tsgl_sound* sound) {
    free(sound->data);
}

#endif