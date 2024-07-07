#include "TSGL_filesystem.h"

size_t tsgl_filesystem_getFileSize(const char* path) {
    FILE* file = fopen(path, "r");
    if (file == NULL) return 0;

    if (fseek(file, 0, SEEK_END) < 0) {
        fclose(file);
        return 0;
    }

    size_t size = ftell(file);
    fclose(file);
    return size;
}