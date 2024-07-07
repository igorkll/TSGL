#pragma once
#if __has_include(<driver/dac.h>)
#include "TSGL.h"
#include <driver/dac.h>

typedef struct {
    bool playing; //get
    bool pause; //get

    size_t position; //set / get
    float speed; //set / get

    void* data;
    size_t len;
    size_t sample_rate;
    size_t bit_rate;
    size_t channels;

    dac_channel_t channel;
} tsgl_sound;

esp_err_t tsgl_sound_load_pcm(tsgl_sound* sound, const char* path, size_t sample_rate, size_t bit_rate, size_t channels); //the bitrate is set not in bits but in bytes
void tsgl_sound_play(tsgl_sound* sound, dac_channel_t channel);
void tsgl_sound_stop(tsgl_sound* sound);
void tsgl_sound_free(tsgl_sound* sound);
#endif