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