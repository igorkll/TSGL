#pragma once
#if __has_include(<driver/dac_oneshot.h>)
    #include <driver/dac_oneshot.h>
    #define HARDWARE_DAC
#endif
#include "TSGL.h"
#include "TSGL_ledc.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gptimer.h>
#include <driver/gpio.h>

//use this instead of the bufferSize to load the track immediately into RAM without loading on playing
#define TSGL_SOUND_FULLBUFFER (2 ^ sizeof(size_t))

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

typedef struct { //do not write ANYTHING in the fields of the structure. use methods. you can only write values to the first two configuration fields
    bool freeAfterPlay; //it will automatically call tsgl_sound_free when the playback is completed
    bool heap; //it will automatically call free when calling tsgl_sound_free

    bool playing;
    float speed;
    float volume;
    bool loop;
    size_t position;

    TaskHandle_t task;
    FILE* file;
    uint8_t* buffer;
    size_t bufferSize;
    size_t bufferPosition;

    size_t len;
    size_t sample_rate;
    size_t bit_rate;
    size_t channels;
    tsgl_sound_pcm_format pcm_format;

    tsgl_sound_output** outputs;
    size_t outputsCount;
    bool freeOutputs;

    gptimer_handle_t timer;
    bool floatAllow;
    bool mute;
    bool reload;
} tsgl_sound;

//the bitrate is set not in bits but in bytes
//however, due to the features of the DAC in esp32, it does not make sense to use more than 8 bit (this will not increase the sound quality)
esp_err_t tsgl_sound_load_pcm(tsgl_sound* sound, size_t bufferSize, int64_t caps, const char* path, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_pcm_format pcm_format);
esp_err_t tsgl_sound_instance(tsgl_sound* sound, tsgl_sound* parent); //it makes a second instance of sound from already loaded data, works only with tracks fully loaded into RAM, it is necessary so that several sound effects can be run simultaneously
void tsgl_sound_setOutputs(tsgl_sound* sound, tsgl_sound_output** outputs, size_t outputsCount, bool freeOutputs);
void tsgl_sound_setSpeed(tsgl_sound* sound, float speed);
void tsgl_sound_setLoop(tsgl_sound* sound, bool loop);
void tsgl_sound_setVolume(tsgl_sound* sound, float volume);
void tsgl_sound_setPosition(tsgl_sound* sound, size_t position);
void tsgl_sound_seek(tsgl_sound* sound, int offset);
void tsgl_sound_play(tsgl_sound* sound);
void tsgl_sound_stop(tsgl_sound* sound);
void tsgl_sound_free(tsgl_sound* sound);

#ifdef HARDWARE_DAC
    tsgl_sound_output* tsgl_sound_newDacOutput(dac_channel_t channel);
#endif
tsgl_sound_output* tsgl_sound_newLedcOutput(gpio_num_t pin);
void tsgl_sound_setOutputValue(tsgl_sound_output* output, uint8_t value);
void tsgl_sound_freeOutput(tsgl_sound_output* output);