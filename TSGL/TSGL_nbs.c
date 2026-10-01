#include "TSGL_nbs.h"
#include "TSGL_filesystem.h"
#include "TSGL_funcs.h"
#include "TSGL_math.h"
#include <math.h>
#include <string.h>
#include <esp_log.h>

static const char* TAG = "TSGL_nbs";

tsgl_nbs_loadedSamples* tsgl_nbs_loadSamples(size_t count, const char* prefix, const char* suffix, size_t sample_rate, size_t bit_rate, size_t channels, tsgl_sound_pcm_format pcm_format) {
    tsgl_nbs_loadedSamples* loadedSamples = malloc(sizeof(tsgl_nbs_loadedSamples));
    if (loadedSamples == NULL) return NULL;

    loadedSamples->count = count;
    loadedSamples->samples = calloc(count, sizeof(tsgl_sound));

    for (size_t i = 0; i < count; i++) {
        char path[TSGL_MAX_PATH_LEN];
        TSGL_funcs_slnprintf(path, TSGL_MAX_PATH_LEN, "%s%i%s", prefix, i, suffix);

        tsgl_sound* sound = &loadedSamples->samples[i];

        if (tsgl_sound_load_pcm(sound, TSGL_SOUND_FULLBUFFER, 0, path, sample_rate, bit_rate, channels, pcm_format) != ESP_OK) {
            tsgl_nbs_freeSamples(loadedSamples);
            return NULL;
        }
    }

    return loadedSamples;
}

void tsgl_nbs_freeSamples(tsgl_nbs_loadedSamples* loadedSamples) {
    for (size_t i = 0; i < loadedSamples->count; i++) {
        tsgl_sound* sound = &loadedSamples->samples[i];
        if (sound != NULL) {
            tsgl_sound_free(sound);
        }
    }

    free(loadedSamples->samples);
    free(loadedSamples);
}

// ---------------------------------------

static uint32_t readInteger(FILE* file) {
    uint32_t result = 0;
    fread(&result, sizeof(uint32_t), 1, file);
    return result;
}

static uint16_t readShort(FILE* file) {
    uint16_t result = 0;
    fread(&result, sizeof(uint16_t), 1, file);
    return result;
}

static uint8_t readByte(FILE* file) {
    uint8_t result = 0;
    fread(&result, sizeof(uint8_t), 1, file);
    return result;
}

static void skipString(FILE* file) {
    uint32_t length = readInteger(file);
    fseek(file, length, SEEK_CUR);
}

// ---------------------------------------

static void _stop(tsgl_nbs* nbs) {
    for (size_t i = 0; i < nbs->active_notes_max; i++) {
        tsgl_sound* active_note = &nbs->active_notes[i];
        if (active_note->playing) {
            tsgl_sound_stop(active_note);
            active_note->userData_int = 1;
        }
    }

    nbs->playing = false;
}

static void _freeNotes(tsgl_nbs* nbs) {
    for (size_t i = 0; i < nbs->active_notes_max; i++) {
        tsgl_sound* active_note = &nbs->active_notes[i];
        if (active_note->inited) tsgl_sound_free_instance(active_note);
    }
}

static void _closeHandle(tsgl_nbs* nbs) {
    if (nbs->file_opened) {
        fclose(nbs->file);
        nbs->file_opened = false;
    }
}

static void _resumeNotes(tsgl_nbs* nbs) {
    for (size_t i = 0; i < nbs->active_notes_max; i++) {
        tsgl_sound* active_note = &nbs->active_notes[i];
        if (active_note->userData_int) {
            tsgl_sound_play(active_note);
            active_note->userData_int = 0;
        }
    }
}

static void _waitActiveNotes(tsgl_nbs* nbs) {
    while (true) {
        bool finded_active_note = false;
        for (size_t i = 0; i < nbs->active_notes_max; i++) {
            tsgl_sound* active_note = &nbs->active_notes[i];
            if (active_note->playing) {
                finded_active_note = true;
                break;
            }
        }
        if (!finded_active_note) break;
        vTaskDelay(1);
    }
}

static void nbs_player_task(tsgl_nbs* nbs) {
    fseek(nbs->file, 0, SEEK_SET);

    uint16_t length = readShort(nbs->file);
    uint16_t tempo;
    bool newFormat = length == 0;
    if (newFormat) {
        readByte(nbs->file); //version
        readByte(nbs->file); //vanilla instrument count
        readShort(nbs->file); //song_length
        readShort(nbs->file); //layer_count
        skipString(nbs->file);
        skipString(nbs->file);
        skipString(nbs->file);
        skipString(nbs->file);
        tempo = readShort(nbs->file);

        for (size_t i = 1; i <= 3; i++) readByte(nbs->file);
        for (size_t i = 1; i <= 5; i++) readInteger(nbs->file);
        skipString(nbs->file);
        for (size_t i = 1; i <= 3; i++) readByte(nbs->file);
    } else {
        readShort(nbs->file); //height
        skipString(nbs->file);
        skipString(nbs->file);
        skipString(nbs->file);
        skipString(nbs->file);
        tempo = readShort(nbs->file);
    
        for (size_t i = 1; i <= 3; i++) readByte(nbs->file);
        for (size_t i = 1; i <= 5; i++) readInteger(nbs->file);
        skipString(nbs->file);
    }

    size_t dataStartPos = ftell(nbs->file);
    float step_ms = 1000.0f / ((float)tempo / 100.0f);

    while (true) {
        uint16_t step = readShort(nbs->file);
        if (step == 0) {
            if (nbs->loop) {
                _waitActiveNotes(nbs);
                fseek(nbs->file, dataStartPos, SEEK_SET);
                continue;
            }
            break;
        }
        if (newFormat) step /= 256;

        while (true) {
            uint16_t jump = readShort(nbs->file);
            if (jump == 0) {
                break;
            }

            if (newFormat) readByte(nbs->file);
            uint8_t inst = readByte(nbs->file);
            uint8_t note = readByte(nbs->file);
            if (newFormat) {
                readByte(nbs->file);
                readShort(nbs->file);
            }

            for (size_t i = 0; i < nbs->active_notes_max; i++) {
                tsgl_sound* active_note = &nbs->active_notes[i];
                if (!active_note->inited) {
                    if (inst < nbs->loadedSamples->count) {
                        tsgl_sound_instance(active_note, &nbs->loadedSamples->samples[inst]);
                        tsgl_sound_enableFreeOnEnd(active_note, true);
                        tsgl_sound_setOutputsRaw(active_note, nbs->outputs, nbs->outputsCount);
                        float speed = pow(2, (note - 45) / 12.0);
                        tsgl_sound_setSpeed(active_note, TSGL_MATH_CLAMP(speed, 0.1, 4));
                        tsgl_sound_setVolume(active_note, nbs->volume);
                        tsgl_sound_play(active_note);
                        active_note->userData_int = 0;
                    } else {
                        ESP_LOGW(TAG, "sample with id %i not loaded", inst);
                    }
                    break;
                }
            }
        }

        TickType_t ticks = pdMS_TO_TICKS((TickType_t)(step_ms * step));
        if (ticks == 0) ticks = 1;
        vTaskDelay(ticks);
    }

    _waitActiveNotes(nbs);

    _stop(nbs);
    _freeNotes(nbs);
    _closeHandle(nbs);

    nbs->task_created = false;
    vTaskDelete(NULL);
}

// ---------------------------------------

esp_err_t tsgl_nbs_load(tsgl_nbs* nbs, tsgl_nbs_loadedSamples* loadedSamples, const char* path, size_t active_notes_max) {
    memset(nbs, 0, sizeof(tsgl_nbs));
    nbs->inited = true;

    nbs->loadedSamples = loadedSamples;
    nbs->path = strdup(path);
    nbs->active_notes_max = active_notes_max;
    nbs->active_notes = calloc(active_notes_max, sizeof(tsgl_sound));

    return ESP_OK;
}

tsgl_nbs* tsgl_nbs_loadHeap(tsgl_nbs_loadedSamples* loadedSamples, const char* path, size_t active_notes_max) {
    tsgl_nbs* nbs = calloc(1, sizeof(tsgl_nbs));
    if (nbs == NULL) return NULL;

    tsgl_nbs_load(nbs, loadedSamples, path, active_notes_max);

    nbs->heap = true;
    return nbs;
}

void tsgl_nbs_play(tsgl_nbs* nbs) {
    if (nbs->playing) return;
    nbs->playing = true;

    if (nbs->task_created) {
        _resumeNotes(nbs);
        vTaskResume(nbs->task);
        return;
    }

    if (!nbs->file_opened) {
        nbs->file = tsgl_filesystem_open(nbs->path, "rb");
        nbs->file_opened = true;
    }
    
    xTaskCreate((TaskFunction_t)nbs_player_task, NULL, TSGL_NBS_STACK_SIZE, nbs, 1, &nbs->task);
    nbs->task_created = true;
}

void tsgl_nbs_stop(tsgl_nbs* nbs) {
    if (!nbs->playing) return;

    if (nbs->task_created) {
		vTaskSuspend(nbs->task);
	}

    _stop(nbs);
}

void tsgl_nbs_stopAndSetPosZero(tsgl_nbs* nbs) {
    if (!nbs->playing) return;

    if (nbs->task_created) {
		vTaskDelete(nbs->task);
        nbs->task_created = false;
	}

    _stop(nbs);
    _freeNotes(nbs);
    _closeHandle(nbs);
}

void tsgl_nbs_setOutputs(tsgl_nbs* nbs, tsgl_sound_output** outputs, size_t outputsCount) {
    nbs->outputs = outputs;
    nbs->outputsCount = outputsCount;
}

void tsgl_nbs_setVolume(tsgl_nbs* nbs, float volume) {
    nbs->volume = volume;

    for (size_t i = 0; i < nbs->active_notes_max; i++) {
        tsgl_sound* active_note = &nbs->active_notes[i];
        tsgl_sound_setVolume(active_note, volume);
    }
}

void tsgl_nbs_setLoop(tsgl_nbs* nbs, bool loop) {
    nbs->loop = loop;
}

void tsgl_nbs_free(tsgl_nbs* nbs) {
    if (nbs->task_created) {
		vTaskDelete(nbs->task);
		nbs->task_created = false;
	}

    _stop(nbs);
    _freeNotes(nbs);
    _closeHandle(nbs);

    if (nbs->path) {
        free(nbs->path);
        nbs->path = NULL;
    }

    if (nbs->active_notes) {
        free(nbs->active_notes);
        nbs->active_notes = NULL;
    }

    memset(nbs, 0, sizeof(tsgl_nbs));
    if (nbs->heap) free(nbs);
}
