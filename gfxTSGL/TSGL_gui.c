#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"
#include "TSGL_gui.h"
#include <esp_random.h>

static tsgl_gui* _createRoot(void* target, bool buffered, tsgl_pos width, tsgl_pos height) {
    tsgl_gui* gui = calloc(1, sizeof(tsgl_gui));
    
    gui->root = gui;
    gui->target = target;
    gui->buffered = buffered;

    gui->interactive = true;
    gui->displayable = true;

    gui->needMath = true;
    gui->needDraw = true;

    gui->math_x = 0;
    gui->math_y = 0;
    
    gui->width = width;
    gui->height = height;
    gui->math_width = width;
    gui->math_height = height;

    return gui;
}

static tsgl_pos _localMath(tsgl_gui_paramFormat format, float val, float max) {
    switch (format) {
        case tsgl_gui_absolute:
            return val + 0.5;

        case tsgl_gui_percent:
            return val * max;
    }
    return 0;
}

static void _math(tsgl_gui* object, tsgl_pos offsetX, tsgl_pos offsetY) {
    if (object->parent != NULL && object->needMath) {
        tsgl_pos localMathX = _localMath(object->format_x, object->x, object->parent->width);
        tsgl_pos localMathY = _localMath(object->format_y, object->y, object->parent->height);
        object->math_x = offsetX + localMathX;
        object->math_y = offsetY + localMathY;
        object->math_width = _localMath(object->format_width, object->width, object->parent->width);
        object->math_height = _localMath(object->format_height, object->height, object->parent->height);

        if (object->math_x < 0) object->math_x = 0;
        if (object->math_y < 0) object->math_y = 0;
        tsgl_pos maxWidth = object->parent->width - localMathX;
        tsgl_pos maxHeight = object->parent->height - localMathY;
        if (object->math_width > maxWidth) object->math_width = maxWidth;
        if (object->math_height > maxHeight) object->math_height = maxHeight;

        object->needMath = false;
    }

    if (object->parents != NULL) {
        for (size_t i = 0; i < object->parentsCount; i++) {
            _math(object->parents[i], object->math_x, object->math_y);
        }
    }
}

static void _initCallback(tsgl_gui* object) {
    if (object->create_callback != NULL) object->create_callback(object);
}

typedef struct {
    void* arg;
    void (*callback) (tsgl_gui* root, void* arg);
} _callback_data;



tsgl_gui* tsgl_gui_createRoot_display(tsgl_display* display, tsgl_colormode colormode) {
    tsgl_gui* gui = _createRoot(display, false, display->width, display->height);
    gui->colormode = colormode;
    _initCallback(gui);
    return gui;
}

tsgl_gui* tsgl_gui_createRoot_buffer(tsgl_framebuffer* framebuffer) {
    tsgl_gui* gui = _createRoot(framebuffer, true, framebuffer->width, framebuffer->height);
    gui->colormode = framebuffer->colormode;
    _initCallback(gui);
    return gui;
}

tsgl_gui* tsgl_gui_addObject(tsgl_gui* object) {
    object->parentsCount++;
    if (object->parents == NULL) {
        object->parents = malloc(object->parentsCount * sizeof(size_t));
    } else {
        object->parents = realloc(object->parents, object->parentsCount * sizeof(size_t));
    }

    tsgl_gui* newObject = calloc(1, sizeof(tsgl_gui));
    newObject->colormode = object->colormode;
    newObject->root = object->root;
    newObject->target = object->target;
    newObject->buffered = object->buffered;
    newObject->parent = object;
    newObject->x = 0;
    newObject->y = 0;
    newObject->width = object->width;
    newObject->height = object->height;
    newObject->interactive = true;
    newObject->displayable = true;
    newObject->needMath = true;
    newObject->needDraw = true;
    object->parents[object->parentsCount - 1] = newObject;
    _initCallback(newObject);
    return newObject;
}

void tsgl_gui_attachClearCallback(tsgl_gui* root, void* arg, void (*onClear)(tsgl_gui* root, void* arg)) {
    _callback_data* callback_data = malloc(sizeof(_callback_data));
    callback_data->arg = arg;
    callback_data->callback = onClear;
    root->data = callback_data;
}

void tsgl_gui_free(tsgl_gui* object) {
    if (object->free_callback) object->free_callback(object);
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



void tsgl_gui_math(tsgl_gui* root) {
    _math(root, 0, 0);
}

void tsgl_gui_draw(tsgl_gui* object) {
    if (!object->displayable || !object->needDraw) return;
    if (object == object->root && object->data) {
        _callback_data* callback_data = (_callback_data*)object->data;
        callback_data->callback(object, callback_data->arg);
    }
    if (object->draw_callback != NULL) object->draw_callback(object);
    if (object->parents != NULL) {
        for (size_t i = 0; i < object->parentsCount; i++) {
            tsgl_gui_draw(object->parents[i]);
        }
    }
    object->needDraw = false;
}



void tsgl_gui_processTouchscreen(tsgl_gui* root, tsgl_touchscreen* touchscreen) {

}

void tsgl_gui_processGui(tsgl_gui* root, void* arg, void (*onDraw)(tsgl_gui* root, void* arg)) {
    tsgl_gui_math(root);

    if (root->needDraw) {
        tsgl_gui_draw(root);
        onDraw(root, arg);
    }
}