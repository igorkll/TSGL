#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"
#include "TSGL_gui.h"
#include <esp_random.h>

static tsgl_gui_object* createRoot(void* target, bool buffered, tsgl_pos width, tsgl_pos height) {
    tsgl_gui_object* gui = calloc(1, sizeof(tsgl_gui_object));
    
    gui->target = target;
    gui->buffered = buffered;

    gui->width = width;
    gui->height = height;

    gui->interactive = true;
    gui->displayable = true;

    return gui;
}

tsgl_gui_object* tsgl_gui_createRoot_display(tsgl_display* display) {
    return createRoot(display, false, display->width, display->height);
}

tsgl_gui_object* tsgl_gui_createRoot_buffer(tsgl_framebuffer* framebuffer) {
    return createRoot(framebuffer, true, framebuffer->width, framebuffer->height);
}

tsgl_gui_object* tsgl_gui_addObject(tsgl_gui_object* object, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    object->parentsCount++;
    if (object->parents == NULL) {
        object->parents = malloc(object->parentsCount * sizeof(size_t));
    } else {
        object->parents = realloc(object->parents, object->parent->parentsCount * sizeof(size_t));
    }

    tsgl_gui_object* newObject = calloc(1, sizeof(tsgl_gui_object));
    newObject->target = object->target;
    newObject->buffered = object->buffered;
    newObject->parent = object;
    newObject->x = x;
    newObject->y = y;
    newObject->width = width;
    newObject->height = height;
    newObject->interactive = true;
    newObject->displayable = true;
    object->parents[object->parentsCount - 1] = newObject;
    return newObject;
}

void tsgl_gui_draw(tsgl_gui_object* object) {
    tsgl_framebuffer_fill(object->target, object->x, object->y, object->width, object->height, (tsgl_rawcolor) {.arr = {esp_random(), esp_random(), esp_random()}});
    if (object->parents != NULL) {
        for (size_t i = 0; i < object->parentsCount; i++) {
            tsgl_gui_draw(object->parents[i]);
        }
    }
}

void tsgl_gui_free(tsgl_gui_object* object) {
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
    free(object);
}