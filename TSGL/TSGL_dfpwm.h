#pragma once
#include "TSGL.h"

typedef struct {
    int fq, q, s, lt;
    int fs_coeff;   // коэффициент LPF (0..256)
    int s_step;     // шаг нарастания силы за семпл
} tsgl_dfpwm_decode_state;

void tsgl_dfpwm_reset(tsgl_dfpwm_decode_state* state, int sample_rate, int cutoff_hz);
int8_t tsgl_dfpwm_decode(tsgl_dfpwm_decode_state* state, uint8_t* buf, size_t bit_pos);
