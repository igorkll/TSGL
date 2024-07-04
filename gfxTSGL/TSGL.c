#include "TSGL.h"
#include <esp_heap_caps.h>
#include <string.h>

const float tsgl_colormodeSizes[] = {2, 2, 2, 2, 3, 3, 1.5, 1.5, 0.125};

size_t tsgl_getPartSize() {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    return info.largest_free_block;
}

#define umin(a,b) (((a) < (b)) ? (a) : (b))
void tsgl_sendFlood(void* arg, void(*send)(void* arg, void* part, size_t size), const uint8_t* data, size_t size, size_t flood) {
    if (size <= 0 || flood <= 0) return;
    size_t part = (tsgl_getPartSize() / size) * size;
    size_t bytesCount = flood * size;
    size_t offset = 0;
    uint8_t* floodPart = malloc(part);
    if (floodPart == NULL) return;
    for (size_t i = 0; i < part; i += size) {
        memcpy(floodPart + i, data, size);
    }
    while (true) {
        send(arg, floodPart, umin(bytesCount - offset, part));
        offset += part;
        if (offset >= bytesCount) {
            break;
        }
    }
    free(floodPart);
}