#include "TSGL_dfpwm.h"

void tsgl_dfpwm_reset(tsgl_dfpwm_decode_state* state) {
    state->fq = 0;
    state->q  = 0;
    state->s  = 0;
    state->lt = -128;
}

int8_t tsgl_dfpwm_decode(tsgl_dfpwm_decode_state* state, uint8_t* buf, size_t bit_pos) {
    int t, nq, lq, st, ns, ov;

    // 1. Извлекаем бит: LSB-first внутри байта
    uint8_t bit = (buf[bit_pos >> 3] >> (bit_pos & 7)) & 1;

    // 2. Целевое значение: 127 для 1, -128 для 0
    t = bit ? 127 : -128;

    // 3. Обновление заряда (charge) с округлением и принудительным сдвигом
    nq = state->q + ((state->s * (t - state->q) + 512) >> 10);
    if (nq == state->q && nq != t) {
        nq += (t == 127 ? 1 : -1);
    }
    lq = state->q;
    state->q = nq;

    // 4. Обновление силы (strength) с границами 8..1023
    st = (t != state->lt) ? 0 : 1023;
    ns = state->s;
    if (ns != st) {
        ns += (st != 0 ? 1 : -1);
    }
    if (ns < 8) ns = 8;
    state->s = ns;

    // 5. Фильтр "Antijerk" — сглаживание резких переходов
    ov = (t != state->lt) ? ((nq + lq + 1) >> 1) : nq;

    // 6. Низкочастотный фильтр (LPF), fs = 140 зашит как в оригинале
    state->fq += ((140 * (ov - state->fq) + 0x80) >> 8);

    // 7. Запоминаем последнюю цель
    state->lt = t;

    // 8. Возвращаем итоговый семпл
    return (int8_t)state->fq;
}