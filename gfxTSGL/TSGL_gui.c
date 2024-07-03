#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_benchmark.h"
#include "TSGL_display.h"
#include "TSGL_gui.h"
#include <esp_random.h>
#include <string.h>
#include <math.h>

static tsgl_gui* _createRoot(void* target, bool buffered, tsgl_pos width, tsgl_pos height) {
    tsgl_gui* gui = calloc(1, sizeof(tsgl_gui));    
    gui->root = gui;
    gui->target = target;
    gui->buffered = buffered;
    gui->leaky_walls = true;
    gui->processing = true;

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

static tsgl_pos _localMath(tsgl_gui_paramFormat format, bool side, float val, float max) {
    switch (format) {
        case tsgl_gui_absolute:
            if (side && val < 0) {
                return max;
            }
            return val + 0.5;

        case tsgl_gui_percent:
            return val * max;
    }
    return 0;
}

static bool _checkIntersection(tsgl_pos x, tsgl_pos y, tsgl_gui* object1, tsgl_gui* object2) {
    return (x < object2->math_x + object2->math_width && 
            x + object1->math_width > object2->math_x && 
            y < object2->math_y + object2->math_height && 
            y + object1->math_height > object2->math_y);
}

static void _math(tsgl_gui* object, tsgl_pos forceOffsetX, tsgl_pos forceOffsetY, bool force) {
    bool forceParentsMath = force;
    if (object->parent != NULL && (object->needMath || forceParentsMath)) {
        tsgl_pos localMathX = _localMath(object->format_x, false, object->x, object->parent->math_width);
        tsgl_pos localMathY = _localMath(object->format_y, false, object->y, object->parent->math_height);
        object->math_x = localMathX;
        object->math_y = localMathY;
        object->math_width = _localMath(object->format_width, true, object->width, object->parent->math_width);
        object->math_height = _localMath(object->format_height, true, object->height, object->parent->math_height);

        if (object->math_x < 0) object->math_x = 0;
        if (object->math_y < 0) object->math_y = 0;
        tsgl_pos maxWidth = object->parent->math_width - localMathX;
        tsgl_pos maxHeight = object->parent->math_height - localMathY;
        if (object->math_width > maxWidth) object->math_width = maxWidth;
        if (object->math_height > maxHeight) object->math_height = maxHeight;

        if (!object->parent->leaky_walls) {
            object->offsetX += localMathX;
            object->offsetY += localMathY;

            if (object->offsetX < 0) {
                object->offsetX = 0;
            } else {
                tsgl_pos maxOffset = object->parent->math_width - object->math_width;
                if (object->offsetX > maxOffset) object->offsetX = maxOffset;
            }
            
            if (object->offsetY < 0) {
                object->offsetY = 0;
            } else {
                tsgl_pos maxOffset = object->parent->math_height - object->math_height;
                if (object->offsetY > maxOffset) object->offsetY = maxOffset;
            }

            object->offsetX -= localMathX;
            object->offsetY -= localMathY;
        }

        object->math_x += object->offsetX + forceOffsetX;
        object->math_y += object->offsetY + forceOffsetY;
        object->processing = _checkIntersection(object->root->math_x, object->root->math_y, object->root, object);

        forceParentsMath = true;
    }
    object->needMath = false;

    if (object->processing) {
        if (object->children != NULL) {
            for (size_t i = 0; i < object->childrenCount; i++) {
                _math(object->children[i], object->math_x, object->math_y, forceParentsMath);
            }
        }
    }
}

static bool _inObjectCheck(tsgl_gui* object, tsgl_pos x, tsgl_pos y) {
    return x >= object->math_x && y >= object->math_y && x < (object->math_x + object->math_width) && y < (object->math_y + object->math_height);
}

static void _toUpLevel(tsgl_gui* object) {
    if (object->parent == NULL) return;
    if (object->draggable) {
        if (object->parent->children[object->parent->childrenCount - 1] != object) {
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
            object->needMath = true;
            object->needDraw = true;
        }
    }
    _toUpLevel(object->parent);
}

static bool _event(tsgl_gui* object, tsgl_pos x, tsgl_pos y, tsgl_gui_event event) {
    if (!object->interactive || !object->processing) {
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
                    _toUpLevel(object);
                }
                break;

            case tsgl_gui_drag:
                if (object->pressed) {
                    if (x != object->tx || y != object->ty) {
                        if (object->draggable) {
                            object->old_math_x = object->math_x;
                            object->old_math_y = object->math_y;
                            object->offsetX = object->tdx + (x - object->tpx);
                            object->offsetY = object->tdy + (y - object->tpy);
                            object->needMath = true;
                            object->needDraw = true;
                            object->localMovent = true;
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

/*
static bool _childrenMathed(tsgl_gui* object, bool mathedReset) {
    bool anyDraw = object->mathed;
    if (mathedReset) object->mathed = false;

    if (object->children != NULL) {
        for (size_t i = 0; i < object->childrenCount; i++) {
            tsgl_gui* child = object->children[i];
            if (child->mathed) {
                anyDraw = true;
                if (mathedReset) child->mathed = false;
            }
        }
    }

    return anyDraw;
}
*/

static void _recursionDrawLater(tsgl_gui* object, tsgl_gui* child, size_t index) {
    for (size_t i = index + 1; i < object->childrenCount; i++) {
        tsgl_gui* child2 = object->children[i];
        if (child != child2 && !child2->drawLater && !child2->drawLaterLater && !child->localMovent && _checkIntersection(child->math_x, child->math_y, child, child2)) {
            child2->drawLater = true;
            child2->needDraw = false;
            _recursionDrawLater(object, child2, i);
        }
    }
}

static bool _draw(tsgl_gui* object, bool force, float dt) {
    if (!object->displayable || !object->processing) {
        object->needDraw = false;
        return false;
    }

    bool anyDraw = false;
    bool forceDraw = force || object->needDraw;
    bool anyDrawLater = false;

    if (object->children != NULL && !forceDraw && object->color.invalid) {
        for (size_t i = 0; i < object->childrenCount; i++) {
            tsgl_gui* child = object->children[i];
            if (child->localMovent) {
                forceDraw = true;
                break;
            }
        }
    }

    if (forceDraw) {
        if (object->localMovent && !force) {
            if (!object->parent->color.invalid) {
                TSGL_GUI_DRAW(object, fill, object->old_math_x, object->old_math_y, object->math_width, object->math_height, object->parent->color);
            }
        }
        object->localMovent = false;

        object->needDraw = false;
        float delta = object->animationTarget - object->animationState;
        if (fabs(delta) > object->animationTolerance) {
            bool animEnd;
            if (delta > 0) {
                object->animationState += object->animationSpeed * dt;
                animEnd = object->animationState > object->animationTarget;
            } else {
                object->animationState -= object->animationSpeed * dt;
                animEnd = object->animationState < object->animationTarget;
            }
            if (animEnd || fabs(object->animationTarget - object->animationState) <= object->animationTolerance) {
                object->animationState = object->animationTarget;
            } else {
                object->needDraw = true;
            }
        }

        if (!object->color.invalid) {
            TSGL_GUI_DRAW(object, fill, object->math_x, object->math_y, object->math_width, object->math_height, object->color);
        } else if (object->draw_callback != NULL) {
            object->draw_callback(object);
        }

        anyDraw = true;
    } else if (object->children != NULL) {
        for (size_t i = 0; i < object->childrenCount; i++) {
            tsgl_gui* child = object->children[i];
            if (child->localMovent) {
                child->drawLaterLater = true;
                for (size_t i = 0; i < object->childrenCount; i++) {
                    tsgl_gui* child2 = object->children[i];
                    if (child != child2 && _checkIntersection(child->old_math_x, child->old_math_y, child, child2)) {
                        child2->drawLater = true;
                        child2->needDraw = false;
                        anyDrawLater = true;
                        _recursionDrawLater(object, child2, i);
                    }
                }
            } else if (child->needDraw && !child->draggable) {
                for (size_t i = 0; i < object->childrenCount; i++) {
                    tsgl_gui* child2 = object->children[i];
                    if (child != child2 && _checkIntersection(child->math_x, child->math_y, child, child2)) {
                        child2->drawLater = true;
                        child2->needDraw = false;
                        anyDrawLater = true;
                    }
                }
            }
        }
    }

    if (object->children != NULL) {
        for (size_t i = 0; i < object->childrenCount; i++) {
            if (_draw(object->children[i], forceDraw, dt)) anyDraw = true;
        }

        if (anyDrawLater) {
            for (size_t i = 0; i < object->childrenCount; i++) {
                tsgl_gui* child = object->children[i];
                if (child->drawLater && _draw(child, true, dt)) {
                    anyDraw = true;
                    child->drawLater = false;
                }
            }

            for (size_t i = 0; i < object->childrenCount; i++) {
                tsgl_gui* child = object->children[i];
                if (child->drawLaterLater && _draw(child, true, dt)) {
                    anyDraw = true;
                    child->drawLaterLater = false;
                }
            }
        }
    }

    return anyDraw;
}



tsgl_gui* tsgl_gui_createRoot_display(tsgl_display* display, tsgl_colormode colormode) {
    tsgl_gui* gui = _createRoot(display, false, display->width, display->height);
    gui->colormode = colormode;
    gui->display = display;
    gui->color = display->black;
    return gui;
}

tsgl_gui* tsgl_gui_createRoot_buffer(tsgl_display* display, tsgl_framebuffer* framebuffer) {
    tsgl_gui* gui = _createRoot(framebuffer, true, framebuffer->width, framebuffer->height);
    gui->colormode = framebuffer->colormode;
    gui->display = display;
    gui->color = framebuffer->black;
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
    newObject->display = object->display;
    newObject->buffered = object->buffered;
    newObject->parent = object;
    newObject->x = 0;
    newObject->y = 0;
    newObject->width = -1; //-1 fills everything without using the percent mode
    newObject->height = -1;
    newObject->interactive = true;
    newObject->displayable = true;
    newObject->needMath = true;
    newObject->needDraw = true;
    newObject->color = TSGL_INVALID_RAWCOLOR;
    newObject->animationSpeed = 0.1;
    newObject->animationTolerance = 0.05;
    object->children[object->childrenCount - 1] = newObject;
    return newObject;
}

void tsgl_gui_free(tsgl_gui* object) {
    if (object->free_callback) object->free_callback(object);
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
    if (object->data != NULL && !object->noFreeData) free(object->data);
    free(object->children);
    free(object);
}



void tsgl_gui_setPosFormat(tsgl_gui* object, tsgl_gui_paramFormat format) {
    object->format_x = format;
    object->format_y = format;
}

void tsgl_gui_setScaleFormat(tsgl_gui* object, tsgl_gui_paramFormat format) {
    object->format_width = format;
    object->format_height = format;
}

void tsgl_gui_setAllFormat(tsgl_gui* object, tsgl_gui_paramFormat format) {
    tsgl_gui_setPosFormat(object, format);
    tsgl_gui_setScaleFormat(object, format);
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

void tsgl_gui_processGui(tsgl_gui* root, tsgl_framebuffer* asyncFramebuffer, tsgl_benchmark* benchmark) {
    if (benchmark != NULL) tsgl_benchmark_startRendering(benchmark);
    _math(root, 0, 0, false);
    bool needSend = _draw(root, false);
    if (benchmark != NULL) tsgl_benchmark_endRendering(benchmark);
    if (needSend && root->buffered) {
        if (benchmark != NULL) tsgl_benchmark_startSend(benchmark);
        if (asyncFramebuffer != NULL) {
            tsgl_display_asyncCopySend(root->display, root->target, asyncFramebuffer);
        } else {
            tsgl_display_send(root->display, root->target);
        }
        if (benchmark != NULL) tsgl_benchmark_endSend(benchmark);
    } else {
        tsgl_benchmark_noSend(benchmark);
    }
}