#include "TSGL.h"

bool TSGL_funcs_hasext(const char* path, const char* ext);
int TSGL_funcs_slnprintf(char *str, size_t size, const char *format, ...);
bool tsgl_funcs_checkIntersection(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_pos x2, tsgl_pos y2, tsgl_pos width2, tsgl_pos height2);
bool tsgl_funcs_checkTouch(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_pos x2, tsgl_pos y2, tsgl_pos width2, tsgl_pos height2);
