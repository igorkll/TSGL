#include "TSGL_math.h"

float math_fmap(float value, float low, float high, float low_2, float high_2) {
    float relative_value = (value - low) / (high - low);
    float scaled_value = low_2 + (high_2 - low_2) * relative_value;
    return scaled_value;
}

int math_imap(int value, int low, int high, int low_2, int high_2) {
    return (int)(math_fmap(value, low, high, low_2, high_2) + 0.5);
}

size_t tsgl_math_maxSendSize(const tsgl_display_settings settings) {
    return settings.width * settings.height * tsgl_colormodeSizes[settings.driver->colormode];
}