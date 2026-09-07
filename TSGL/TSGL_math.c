#include "TSGL_math.h"
#include <stdint.h>

float tsgl_math_fmap(float value, float low, float high, float low_2, float high_2) {
    float relative_value = (value - low) / (high - low);
    float scaled_value = low_2 + (high_2 - low_2) * relative_value;
    return scaled_value;
}

int tsgl_math_imap(int value, int low, int high, int low_2, int high_2) {
    int64_t num = (int64_t)(value - low) * (high_2 - low_2);
    int64_t den = (int64_t)(high - low);
    
    // Если знаменатель отрицательный, меняем знак у числителя и знаменателя
    // (чтобы работать с положительным знаменателем для упрощения округления)
    if (den < 0) {
        num = -num;
        den = -den;
    }
    
    // Округление к ближайшему:
    // Добавляем половину знаменателя к числителю с учётом знака num
    if (num >= 0) {
        num += den / 2;          // для положительных
    } else {
        num -= den / 2;          // для отрицательных (важно!)
    }
    
    // Целочисленное деление с truncation к нулю (но из-за добавления половины даёт нужное округление)
    int64_t result = num / den + low_2;
    
    return (int)result;
}

size_t tsgl_math_maxSendSize(const tsgl_display_settings settings) {
    return settings.width * settings.height * tsgl_colormodeSizes[settings.driver->colormode];
}