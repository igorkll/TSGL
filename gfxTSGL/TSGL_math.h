#pragma once

#define TSGL_MATH_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define TSGL_MATH_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define TSGL_MATH_CLAMP(val,lower,upper) (TSGL_MATH_MAX(lower, TSGL_MATH_MIN(val, upper)))