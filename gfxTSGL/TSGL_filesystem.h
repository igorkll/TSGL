#include "TSGL.h"

esp_err_t tsgl_filesystem_mount_fatfs(const char* path, const char* name);

size_t tsgl_filesystem_getFileSize(const char* path);