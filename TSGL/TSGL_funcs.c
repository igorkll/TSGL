#include "TSGL_funcs.h"
#include <stdarg.h>
#include <string.h>

bool TSGL_funcs_hasext(const char* path, const char* ext) {
    size_t plen = strlen(path);
    size_t elen = strlen(ext);
    if (plen < elen) return false;
    const char* p = path + plen - elen;
    for (size_t i = 0; i < elen; i++) {
        char a = p[i];
        char b = ext[i];
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return false;
    }
    return true;
}

int TSGL_funcs_slnprintf(char *str, size_t size, const char *format, ...) {
    int result;
    va_list args;

    va_start(args, format);
    result = vsnprintf(str, size, format, args);
    va_end(args);

    // если размер итоговой строки привышает size
    // стандартная snprintf не записывает \0
    // данная функция исправляет это
    if (size > 0) {
        str[size - 1] = '\0';
    }

    return result;
}

bool tsgl_funcs_checkIntersection(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_pos x2, tsgl_pos y2, tsgl_pos width2, tsgl_pos height2) {
    return (x < x2 + width2 && 
            x + width > x2 && 
            y < y2 + height2 && 
            y + height > y2);
}

bool tsgl_funcs_checkTouch(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_pos x2, tsgl_pos y2, tsgl_pos width2, tsgl_pos height2) {
    return (x <= x2 + width2 && 
            x + width >= x2 && 
            y <= y2 + height2 && 
            y + height >= y2);
}
