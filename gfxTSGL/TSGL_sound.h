#pragma once
#if __has_include(<driver/dac_oneshot.h>)
    #include <driver/dac_oneshot.h>
    #define HARDWARE_DAC
#endif
#include "TSGL.h"
#include "TSGL_ledc.h"
#include <driver/timer.h>
#include <driver/gpio.h>

typedef struct {
    #ifdef HARDWARE_DAC
        dac_oneshot_handle_t* channel;
    #endif
    tsgl_ledc* ledc;
} tsgl_sound_output;

typedef enum {
    tsgl_sound_pcm_unsigned,
    tsgl_sound_pcm_signed
} tsgl_sound_pcm_format;

typedef struct {
    bool freeAfterPlay; //it will automatically call tsgl_sound_free when the playback is completed
    bool heap; //it will automatically call free when calling tsgl_sound_free

    //get only
    bool playing; 
    bool pause;
    float speed;
    float volume;
    bool mute;
    bool floatAllow;
    bool loop;
    size_t position;

    uint8_t* data;
    size_t len;
    size_t sample_rate;
    size_t bit_rate;
    size_t channels;
    tsgl_sound_pcm_format pcm_format;

    tsgl_sound_output** outputs;
    size_t outputsCount;
    bool freeOutputs;

    timer_idx_t timer;
    timer_group_t timerGroup;
} tsgl_sound;

//the bitrate is set not in bits but in bytes
//however, due to the features of the DAC in esp32, it does not make sense to use more than 8 bit (this will not increase the sound quality)
esp_err_t tsgl_sound_load_pcm(tsgl_sound* sound, int64_t caps, const char* path, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_pcm_format pcm_format);
void tsgl_sound_setOutputs(tsgl_sound* sound, tsgl_sound_output** outputs, size_t outputsCount, bool freeOutputs);
void tsgl_sound_setSpeed(tsgl_sound* sound, float speed);
void tsgl_sound_setPause(tsgl_sound* sound, bool pause);
void tsgl_sound_setLoop(tsgl_sound* sound, bool loop);
void tsgl_sound_setVolume(tsgl_sound* sound, float volume);
void tsgl_sound_play(tsgl_sound* sound);
void tsgl_sound_stop(tsgl_sound* sound);
void tsgl_sound_free(tsgl_sound* sound);

#ifdef HARDWARE_DAC
    tsgl_sound_output* tsgl_sound_newDacOutput(dac_channel_t channel);
#endif
tsgl_sound_output* tsgl_sound_newLedcOutput(gpio_num_t pin);
void tsgl_sound_setOutputValue(tsgl_sound_output* output, uint8_t value);
void tsgl_sound_freeOutput(tsgl_sound_output* output);