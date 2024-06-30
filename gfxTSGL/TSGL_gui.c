#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_display.h"
#include "TSGL_gui.h"
#include <esp_random.h>
#include <string.h>

typedef struct {
    void* arg;
    void (*callback) (tsgl_gui* root, void* arg);
    bool free_arg;
} _callback_data;

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

static void _math(tsgl_gui* object, tsgl_pos forceOffsetX, tsgl_pos forceOffsetY, bool force) {
    bool forceParentsMath = force;
    if (object->parent != NULL && (object->needMath || forceParentsMath)) {
        tsgl_pos localMathX = _localMath(object->format_x, object->x, object->parent->width);
        tsgl_pos localMathY = _localMath(object->format_y, object->y, object->parent->height);
        object->math_x = localMathX;
        object->math_y = localMathY;
        object->math_width = _localMath(object->format_width, object->width, object->parent->width);
        object->math_height = _localMath(object->format_height, object->height, object->parent->height);

        if (object->math_x < 0) object->math_x = 0;
        if (object->math_y < 0) object->math_y = 0;
        tsgl_pos maxWidth = object->parent->width - localMathX;
        tsgl_pos maxHeight = object->parent->height - localMathY;
        if (object->math_width > maxWidth) object->math_width = maxWidth;
        if (object->math_height > maxHeight) object->math_height = maxHeight;

        if (object->parent != object->root) {
            object->offsetX += localMathX;
            object->offsetY += localMathY;

            if (object->offsetX < 0) {
                object->offsetX = 0;
            } else {
                tsgl_pos maxOffset = object->parent->width - object->math_width;
                if (object->offsetX > maxOffset) object->offsetX = maxOffset;
            }
            
            if (object->offsetY < 0) {
                object->offsetY = 0;
            } else {
                tsgl_pos maxOffset = object->parent->height - object->math_height;
                if (object->offsetY > maxOffset) object->offsetY = maxOffset;
            }

            object->offsetX -= localMathX;
            object->offsetY -= localMathY;
        }

        object->math_x += object->offsetX + forceOffsetX;
        object->math_y += object->offsetY + forceOffsetY;

        forceParentsMath = true;
        object->mathed = true;
    }
    object->needMath = false;

    if (object->children != NULL) {
        for (size_t i = 0; i < object->childrenCount; i++) {
            _math(object->children[i], object->math_x, object->math_y, forceParentsMath);
        }
    }
}

static void _initCallback(tsgl_gui* object) {
    if (object->create_callback != NULL) object->create_callback(object);
}

static void _fillObject(tsgl_gui* root, void* _color) {
    tsgl_rawcolor* color = _color;
    if (root->buffered) {
        tsgl_framebuffer_fill(root->target, root->math_x, root->math_y, root->math_width, root->math_height, *color);
    } else {
        tsgl_display_fill(root->target, root->math_x, root->math_y, root->math_width, root->math_height, *color);
    }
}

static bool _inObjectCheck(tsgl_gui* object, tsgl_pos x, tsgl_pos y) {
    return x >= object->math_x && y >= object->math_y && x < (object->math_x + object->math_width) && y < (object->math_y + object->math_height);
}

static void _toUpLevel(tsgl_gui* object) {
    if (object->parent != NULL && object->parent->children[object->parent->childrenCount - 1] != object) {
        for (size_t i = 0; i < object->parent->childrenCount; i++) {
            if (object->parent->children[i] == object) {
                i++;
                for (; i < object->parent->childrenCount; i++) {
                    object->parent->children[i - 1] = object->parent->children[i];
                }
                break;
            }
        }
        object->parent->children[object->parent->childrenCount - 1] = object;
    }
}

static bool _event(tsgl_gui* object, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {
    if (!object->interactive) {
        object->pressed = false;
        return false;
    }

    if (object->children != NULL) {
        for (int i = object->childrenCount - 1; i >= 0; i--) {
            if (_event(object->children[i], x, y, event)) return true;
        }
    }

    if (object->root != object) {
        switch (event) {
            case tsgl_gui_click:
                if (!object->pressed && _inObjectCheck(object, x, y)) {
                    if (object->event_callback != NULL)
                        object->event_callback(object, x - object->math_x, y - object->math_y, event);
                    object->tpx = x;
                    object->tpy = y;
                    object->tdx = object->offsetX;
                    object->tdy = object->offsetY;
                    object->pressed = true;
                    if (object->draggable) _toUpLevel(object);
                }
                break;

            case tsgl_gui_drag:
                if (object->pressed) {
                    if (x != object->tx || y != object->ty) {
                        if (object->draggable) {
                            object->offsetX = object->tdx + (x - object->tpx);
                            object->offsetY = object->tdy + (y - object->tpy);
                            object->needMath = true;
                            object->needDraw = true;
                        } else if (object->event_callback != NULL) {
                            object->event_callback(object, x - object->math_x, y - object->math_y, event);
                        }
                        object->tx = x;
                        object->ty = y;
                    }
                }
                break;

            case tsgl_gui_drop:
                if (object->pressed) {
                    if (object->event_callback != NULL)
                        object->event_callback(object, x - object->math_x, y - object->math_y, event);
                    object->pressed = false;
                }
                break;
        }
    }
    
    return object->pressed;
}

static bool _needDrawTree(tsgl_gui* object, bool mathedReset) {
    bool anyDraw = object->mathed;
    if (mathedReset) object->mathed = false;

    if (object->children != NULL && !anyDraw) {
        for (size_t i = 0; i < object->childrenCount; i++) {
            if (_needDrawTree(object->children[i], mathedReset)) anyDraw = true;
        }
    }

    return anyDraw;
}

static bool _draw(tsgl_gui* object, bool force) {
    if (!object->displayable) {
        object->needDraw = false;
        return false;
    }

    bool anyDraw = false;
    bool forceDraw = force || object->needDraw || _needDrawTree(object, true);

    if (forceDraw) {
        if (object->predrawData != NULL) {
            _callback_data* callback_data = (_callback_data*)object->predrawData;
            callback_data->callback(object, callback_data->arg);
        }

        if (object->draw_callback != NULL)
            object->draw_callback(object);

        object->needDraw = false;
        anyDraw = true;
    }

    if (object->children != NULL) {
        for (size_t i = 0; i < object->childrenCount; i++) {
            if (_draw(object->children[i], forceDraw)) anyDraw = true;
        }
    }

    return anyDraw;
}



tsgl_gui* tsgl_gui_createRoot_display(tsgl_display* display, tsgl_colormode colormode) {
    tsgl_gui* gui = _createRoot(display, false, display->width, display->height);
    gui->colormode = colormode;
    gui->display = display;
    _initCallback(gui);
    return gui;
}

tsgl_gui* tsgl_gui_createRoot_buffer(tsgl_display* display, tsgl_framebuffer* framebuffer) {
    tsgl_gui* gui = _createRoot(framebuffer, true, framebuffer->width, framebuffer->height);
    gui->colormode = framebuffer->colormode;
    gui->display = display;
    _initCallback(gui);
    return gui;
}

tsgl_gui* tsgl_gui_addObject(tsgl_gui* object) {
    object->childrenCount++;
    if (object->children == NULL) {
        object->children = malloc(object->childrenCount * sizeof(size_t));
    } else {
        object->children = realloc(object->children, object->childrenCount * sizeof(size_t));
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
    object->children[object->childrenCount - 1] = newObject;
    _initCallback(newObject);
    return newObject;
}

void tsgl_gui_setColor(tsgl_gui* object, tsgl_rawcolor color) {
    tsgl_rawcolor* mColor = (tsgl_rawcolor*)malloc(sizeof(tsgl_rawcolor));
    memcpy(mColor, &color, sizeof(tsgl_rawcolor));
    tsgl_gui_attachPredrawCallback(object, true, mColor, _fillObject);
}

void tsgl_gui_attachPredrawCallback(tsgl_gui* object, bool free_arg, void* arg, void (*predraw)(tsgl_gui* root, void* arg)) {
    if (object->predrawData != NULL) {
        _callback_data* callback_data = (_callback_data*)object->predrawData;
        if (callback_data->free_arg) free(callback_data->arg);
        free(object->predrawData);
    }

    _callback_data* callback_data = malloc(sizeof(_callback_data));
    callback_data->arg = arg;
    callback_data->callback = predraw;
    callback_data->free_arg = free_arg;
    object->predrawData = callback_data;
}

void tsgl_gui_free(tsgl_gui* object) {
    if (object->free_callback) object->free_callback(object);
    if (object->predrawData != NULL) {
        _callback_data* callback_data = (_callback_data*)object->predrawData;
        if (callback_data->free_arg) free(callback_data->arg);
        free(object->predrawData);
    }
    if (object->children != NULL) {
        for (size_t i = 0; i < object->childrenCount; i++) {
            tsgl_gui_free(object->children[i]);
        }
    }
    if (object->parent != NULL) {
        for (size_t i = 0; i < object->parent->childrenCount; i++) {
            if (object->parent->children[i] == object) {
                i++;
                for (; i < object->parent->childrenCount; i++) {
                    object->parent->children[i - 1] = object->parent->children[i];
                }
                object->parent->childrenCount--;
                object->parent->children = realloc(object->parent->children, object->parent->childrenCount * sizeof(size_t));
                break;
            }
        }
    }
    if (object->data != NULL) free(object->data);
    free(object->children);
    free(object);
}



void tsgl_gui_processTouchscreen(tsgl_gui* root, tsgl_touchscreen* touchscreen) {
    uint8_t touchCount = tsgl_touchscreen_touchCount(touchscreen);
    if (touchCount > 0) {
        tsgl_touchscreen_point point = tsgl_touchscreen_getPoint(touchscreen, 0);
        if (root->pressed) {
            if (point.x != root->tx || point.y != root->ty) {
                root->tx = point.x;
                root->ty = point.y;
                _event(root, point.x, point.y, tsgl_gui_drag);
            }
        } else {
            root->tx = point.x;
            root->ty = point.y;
            _event(root, point.x, point.y, tsgl_gui_click);
        }
        root->pressed = true;
    } else if (root->pressed) {
        _event(root, root->tx, root->ty, tsgl_gui_drop);
        root->pressed = false;
    }
}

void tsgl_gui_processGui(tsgl_gui* root, tsgl_framebuffer* asyncFramebuffer) {
    if (_draw(root, false) && root->buffered) {
        _math(root, 0, 0, false);

        if (asyncFramebuffer != NULL) {
            if (root->needDraw || _needDrawTree(root, false)) {
                tsgl_display_asyncSend(root->display, root->target, asyncFramebuffer);
            } else {
                tsgl_display_send(root->display, root->target);
            }
        } else {
            tsgl_display_send(root->display, root->target);
        }
    } else {
        _math(root, 0, 0, false);
    }
}