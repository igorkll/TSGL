#pragma once
#include "TSGL.h"

#define TSGL_MATH_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define TSGL_MATH_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define TSGL_MATH_CLAMP(val,lower,upper) (TSGL_MATH_MAX(lower, TSGL_MATH_MIN(val, upper)))

float tsgl_math_fmap(float value, float low, float high, float low_2, float high_2);
int tsgl_math_imap(int value, int low, int high, int low_2, int high_2);
size_t tsgl_math_maxSendSize(const tsgl_display_settings settings);