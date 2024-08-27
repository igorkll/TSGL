#include "TSGL_filesystem.h"
#include <esp_vfs.h>
#include <esp_vfs_fat.h>
#include <dirent.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>

esp_err_t tsgl_filesystem_mount_fatfs(const char* path, const char* name) {
    static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
    esp_vfs_fat_mount_config_t storage_mount_config = {
        .max_files = 4,
        .format_if_mount_failed = false,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };

    return esp_vfs_fat_spiflash_mount_rw_wl(path, name, &storage_mount_config, &s_wl_handle);
}


FILE* tsgl_filesystem_open(const char* path, const char* mode) {
    return fopen(path, mode);
}

size_t tsgl_filesystem_writeFile(const char* path, void* buffer, size_t bufferLen) {
    FILE *file = tsgl_filesystem_open(path, "wb");
    if (file == NULL) return 0;
    size_t size = fwrite(buffer, 1, bufferLen, file);
    fclose(file);
    return size;
}

size_t tsgl_filesystem_readFile(const char *path, void* buffer, size_t bufferLen) {
    FILE *file = tsgl_filesystem_open(path, "rb");
    if (file == NULL) return 0;
    size_t size = fread(buffer, 1, bufferLen, file);
    fclose(file);
    return size;
}


bool tsgl_filesystem_exists(const char *path) {
    struct stat state;
    return stat(path, &state) == 0;
}

bool tsgl_filesystem_isDirectory(const char *path) {
    struct stat state;
    if(stat(path, &state) == 0) {
        if (state.st_mode & S_IFDIR) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

size_t tsgl_filesystem_size(const char* path) {
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