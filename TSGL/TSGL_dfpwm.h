#pragma once
#include "TSGL.h"

typedef struct {
    int fq;  // Состояние низкочастотного фильтра (LPF)
    int q;   // Текущий заряд (charge)
    int s;   // Текущая сила (strength)
    int lt;  // Последняя цель (last target): 127 или -128
} tsgl_dfpwm_decode_state;

void tsgl_dfpwm_reset(tsgl_dfpwm_decode_state* state);
int8_t tsgl_dfpwm_decode(tsgl_dfpwm_decode_state* state, uint8_t* buf, size_t bit_pos);
