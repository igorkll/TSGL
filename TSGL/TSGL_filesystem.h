#include "TSGL.h"

esp_err_t tsgl_filesystem_mount_fatfs(const char* path, const char* name);

FILE* tsgl_filesystem_open(const char* path, const char* mode);
size_t tsgl_filesystem_writeFile(const char* path, void* buffer, size_t bufferLen);
size_t tsgl_filesystem_readFile(const char *path, void* buffer, size_t bufferLen);

bool tsgl_filesystem_exists(const char *path);
bool tsgl_filesystem_isDirectory(const char *path);
size_t tsgl_filesystem_size(const char* path);