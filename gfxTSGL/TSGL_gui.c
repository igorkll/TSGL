#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"
#include "TSGL_gui.h"
#include <esp_random.h>

static tsgl_gui* _createRoot(void* target, bool buffered, tsgl_pos width, tsgl_pos height) {
    tsgl_gui* gui = calloc(1, sizeof(tsgl_gui));
    
    gui->target = target;
    gui->buffered = buffered;

    gui->width = width;
    gui->height = height;

    gui->interactive = true;
    gui->displayable = true;

    return gui;
}

static void _math(tsgl_gui* object, tsgl_pos offsetX, tsgl_pos offsetY) {
    object->math_x = offsetX + object->x;
    object->math_y = offsetY + object->y;
    object->math_width = object->width;
    object->math_height = object->height;
    if (object->parents != NULL) {
        for (size_t i = 0; i < object->parentsCount; i++) {
            _math(object->parents[i], object->math_x, object->math_y);
        }
    }
}


tsgl_gui* tsgl_gui_createRoot_display(tsgl_display* display) {
    return _createRoot(display, false, display->width, display->height);
}

tsgl_gui* tsgl_gui_createRoot_buffer(tsgl_framebuffer* framebuffer) {
    return _createRoot(framebuffer, true, framebuffer->width, framebuffer->height);
}

tsgl_gui* tsgl_gui_addObject(tsgl_gui* object) {
    object->parentsCount++;
    if (object->parents == NULL) {
        object->parents = malloc(object->parentsCount * sizeof(size_t));
    } else {
        object->parents = realloc(object->parents, object->parentsCount * sizeof(size_t));
    }

    tsgl_gui* newObject = calloc(1, sizeof(tsgl_gui));
    newObject->target = object->target;
    newObject->buffered = object->buffered;
    newObject->parent = object;
    newObject->x = 0;
    newObject->y = 0;
    newObject->width = object->width;
    newObject->height = object->height;
    newObject->interactive = true;
    newObject->displayable = true;
    object->parents[object->parentsCount - 1] = newObject;
    return newObject;
}

void tsgl_gui_math(tsgl_gui* object) {
    _math(object, 0, 0);
}

void tsgl_gui_draw(tsgl_gui* object) {
    tsgl_framebuffer_fill(
        object->target,
        object->math_x,
        object->math_y,
        object->math_width,
        object->math_height,
        (tsgl_rawcolor) {.arr = {esp_random(), esp_random(), esp_random()}}
    );

    if (object->parents != NULL) {
        for (size_t i = 0; i < object->parentsCount; i++) {
            tsgl_gui_draw(object->parents[i]);
        }
    }
}

void tsgl_gui_free(tsgl_gui* object) {
    if (object->data != NULL) free(object->data);
    if (object->parents != NULL) {
        for (size_t i = 0; i < object->parentsCount; i++) {
            tsgl_gui_free(object->parents[i]);
        }
    }
    if (object->parent != NULL) {
        for (size_t i = 0; i < object->parent->parentsCount; i++) {
            if (object->parent->parents[i] == object) {
                i++;
                for (; i < object->parent->parentsCount; i++) {
                    object->parent->parents[i - 1] = object->parent->parents[i];
                }
                object->parent->parentsCount--;
                object->parent->parents = realloc(object->parent->parents, object->parent->parentsCount * sizeof(size_t));
                break;
            }
        }
    }
    free(object->parents);
    free(object);
}