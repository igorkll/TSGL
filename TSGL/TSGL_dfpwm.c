#include "TSGL_dfpwm.h"
#include <math.h>

void tsgl_dfpwm_reset(tsgl_dfpwm_decode_state* state,
    int sample_rate,
    int cutoff_hz) {
    state->fq = 0;
    state->q  = 0;
    state->s  = 0;
    state->lt = -128;

    // LPF: alpha = 1 - exp(-2*pi*fc/Fs), fs_coeff = alpha*256
    double alpha = 1.0 - exp(-2.0 * M_PI * cutoff_hz / sample_rate);
    int fc = (int)(alpha * 256.0 + 0.5);
    if (fc < 1)   fc = 1;
    if (fc > 256) fc = 256;
    state->fs_coeff = fc;

    // s-ramp: сохраняем время атаки ≈ 21 мс (как в оригинале на 48 кГц)
    double attack_samples = 0.021 * sample_rate;   // 21 мс в семплах
    int step = (int)(1015.0 / attack_samples + 0.5);
    if (step < 1) step = 1;
    state->s_step = step;
}

int8_t tsgl_dfpwm_decode(tsgl_dfpwm_decode_state* state,
    uint8_t* buf, size_t bit_pos)
{
    int t, nq, lq, st, ns, ov;

    uint8_t bit = (buf[bit_pos >> 3] >> (bit_pos & 7)) & 1;
    t = bit ? 127 : -128;

    // q update — без изменений (s уже масштабирован)
    nq = state->q + ((state->s * (t - state->q) + 512) >> 10);
    if (nq == state->q && nq != t)
    nq += (t == 127 ? 1 : -1);
    lq = state->q;
    state->q = nq;

    // s update — с масштабированным шагом
    st = (t != state->lt) ? 0 : 1023;
    ns = state->s;
    if (ns != st) {
    ns += (st != 0 ? state->s_step : -state->s_step);
    }
    if (ns < 8)     ns = 8;
    if (ns > 1023)  ns = 1023;
    state->s = ns;

    // antijerk
    ov = (t != state->lt) ? ((nq + lq + 1) >> 1) : nq;

    // LPF — с динамическим коэффициентом
    state->fq += ((state->fs_coeff * (ov - state->fq) + 0x80) >> 8);

    state->lt = t;
    return (int8_t)state->fq;
}
