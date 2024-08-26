#include "TSGL.h"
#include "TSGL_math.h"
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <string.h>

static const char* TAG = "TSGL";
const float tsgl_colormodeSizes[] = {2, 2, 2, 2, 3, 3, 1.5, 1.5, 0.125};

size_t tsgl_getPartSize() {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    if (info.largest_free_block != info.minimum_free_bytes) {
        return info.largest_free_block;
    }
    return info.largest_free_block / 2;
}

void tsgl_sendFlood(size_t maxPart, void* arg, bool(*send)(void* arg, void* part, size_t size), const uint8_t* data, size_t size, size_t flood) {
    if (size <= 0 || flood <= 0) return;
    size_t part = (tsgl_getPartSize() / size) * size;
    if (maxPart != 0 && part > maxPart) part = maxPart;
    size_t bytesCount = flood * size;
    size_t offset = 0;
    uint8_t* floodPart = malloc(part);
    if (floodPart == NULL) return;
    for (size_t i = 0; i < part; i += size) {
        memcpy(floodPart + i, data, size);
    }
    while (true) {
        if (!send(arg, floodPart, TSGL_MATH_MIN(bytesCount - offset, part))) {
            part /= 2;
            continue;
        }
        offset += part;
        if (offset >= bytesCount) {
            break;
        }
    }
    free(floodPart);
}

void* tsgl_malloc(size_t size, int64_t caps) {
    if (caps == 0) {
        return malloc(size);
    } else {
        void* buffer = heap_caps_malloc(size, caps);
        if (buffer == NULL) {
            ESP_LOGW(TAG, "failed to allocate buffer with caps. attempt to allocate without caps");
            return malloc(size);
        }
        return buffer;
    }
}