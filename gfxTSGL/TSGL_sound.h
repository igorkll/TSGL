#pragma once
#if __has_include(<driver/dac.h>)
#include "TSGL.h"
#include <driver/dac.h>

typedef struct {
    bool playing; //get
    bool pause; //get
    
    bool loop; //set / get
    size_t position; //set / get
    float speed; //set / get

    void* data;
    size_t len;
    size_t sample_rate;
    size_t bit_rate;
    size_t channels;

    dac_channel_t channel;
    dac_channel_t channel2;

    timer_idx_t timer;
    timer_group_t timerGroup;
} tsgl_sound;

//the bitrate is set not in bits but in bytes
//however, due to the features of the DAC in esp32, it does not make sense to use more than 8 bit (this will not increase the sound quality)
esp_err_t tsgl_sound_load_pcm(tsgl_sound* sound, const char* path, size_t sample_rate, size_t bit_rate, size_t channels);
void tsgl_sound_play(tsgl_sound* sound, dac_channel_t channel, dac_channel_t channel2);
void tsgl_sound_stop(tsgl_sound* sound);
void tsgl_sound_free(tsgl_sound* sound);

void tsgl_sound_pushPcm(const char* path, size_t sample_rate, size_t bit_rate, size_t channels, float speed, dac_channel_t channel, dac_channel_t channel2);
#endif