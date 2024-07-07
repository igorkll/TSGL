#if __has_include(<driver/dac.h>)
#include "TSGL_sound.h"
#include "TSGL_filesystem.h"
#include <driver/dac.h>
#include <driver/timer.h>
#include <stdio.h>

esp_err_t tsgl_sound_load_raw(tsgl_sound* sound, const char* path, size_t sample_rate, size_t bit_rate, size_t channels) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) return ESP_FAIL;
    sound->len = tsgl_filesystem_getFileSize(path);
    sound->sample_rate = sample_rate;
    sound->bit_rate = bit_rate;
    sound->channels = channels;
    fread(sound->data, sound->bit_rate, sound->len, file);
    fclose(file);
    return ESP_OK;
}

void tsgl_sound_free(tsgl_sound* sound) {
    free(sound->data);
}

#endif