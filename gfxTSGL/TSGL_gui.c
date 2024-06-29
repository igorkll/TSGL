#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"
#include "TSGL_gui.h"

tsgl_gui* tsgl_gui_createForDisplay(tsgl_display* display) {
    tsgl_gui* gui = malloc(sizeof(tsgl_gui));
    gui->parents = NULL;
    gui->target = display;
    gui->buffered = false;
    return gui;
}

tsgl_gui* tsgl_gui_createForBuffer(tsgl_framebuffer* framebuffer) {
    tsgl_gui* gui = malloc(sizeof(tsgl_gui));
    gui->parents = NULL;
    gui->target = framebuffer;
    gui->buffered = true;
    return gui;
}

void tsgl_gui_free(tsgl_gui_object* object) {
    if (object->data != NULL) free(object->data);
    if (object->parents != NULL) {
        for (size_t i = 0; i < object->parentsCount; i++) {
            tsgl_gui_freeObject(object->parents[i]);
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
                object->parent->parents = realloc(object->parent->parents, object->parent->parentsCount * sizeof(tsgl_gui_object));
                break;
            }
        }
    }
    free(object);
}