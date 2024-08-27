#include "TSGL.h"
#include "TSGL_touchscreen.h"
#include "TSGL_framebuffer.h"
#include "TSGL_benchmark.h"
#include "TSGL_display.h"
#include "TSGL_gui.h"
#include "TSGL_math.h"
#include <esp_random.h>
#include <string.h>
#include <math.h>

static tsgl_gui* _createRoot(void* target, bool buffered, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    tsgl_gui* gui = calloc(1, sizeof(tsgl_gui));    
    gui->root = gui;
    gui->target = target;
    gui->buffered = buffered;
    gui->leaky_walls = false;
    gui->processing = true;

    gui->interactive = true;
    gui->displayable = true;

    gui->needMath = true;
    gui->needDraw = true;

    gui->x = x;
    gui->y = y;
    gui->math_x = x;
    gui->math_y = y;
    
    gui->width = width;
    gui->height = height;
    gui->math_width = width;
    gui->math_height = height;
    return gui;
}

static tsgl_pos _localMath(tsgl_gui_paramFormat format, uint8_t valType, float val, float max, tsgl_pos minSide, tsgl_pos maxSide, tsgl_pos width, tsgl_pos height) {
    switch (format) {
        case tsgl_gui_absolute:
            if (valType == 1 && val < 0) {
                return max;
            }
            return val + 0.5;

        case tsgl_gui_percent:
            return val * max;

        case tsgl_gui_percentMinSide:
            return val * minSide;

        case tsgl_gui_percentMaxSide:
            return val * maxSide;

        case tsgl_gui_percentWidth:
            return val * width;

        case tsgl_gui_percentheight:
            return val * height;
    }
    return 0;
}

static bool _checkIntersection(tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height, tsgl_gui* object2) {
    return (x < object2->math_x + object2->math_width && 
            x + width > object2->math_x && 
            y < object2->math_y + object2->math_height && 
            y + height > object2->math_y);
}

static void _math(tsgl_gui* object, tsgl_pos forceOffsetX, tsgl_pos forceOffsetY, bool force) {
    bool forceParentsMath = force;
    if (object->parent != NULL && (object->needMath || forceParentsMath)) {
        tsgl_pos mWidth = object->parent->math_width;
        tsgl_pos mHeight = object->parent->math_height;
        tsgl_pos minSide = TSGL_MATH_MIN(mWidth, mHeight);
        tsgl_pos maxSide = TSGL_MATH_MAX(mWidth, mHeight);
        tsgl_pos localMathX = _localMath(object->format_x, 0, object->x, object->parent->math_width, minSide, maxSide, mWidth, mHeight);
        tsgl_pos localMathY = _localMath(object->format_y, 0, object->y, object->parent->math_height, minSide, maxSide, mWidth, mHeight);
        object->math_x = localMathX;
        object->math_y = localMathY;
        object->math_width = _localMath(object->format_width, 1, object->width, object->parent->math_width, minSide, maxSide, mWidth, mHeight);
        object->math_height = _localMath(object->format_height, 1, object->height, object->parent->math_height, minSide, maxSide, mWidth, mHeight);

        object->math_min_width = _localMath(object->format_min_width,   2, object->min_width,  object->parent->math_width, minSide, maxSide, mWidth, mHeight);
        object->math_min_height = _localMath(object->format_min_height, 2, object->min_height, object->parent->math_height, minSide, maxSide, mWidth, mHeight);
        object->math_max_width = _localMath(object->format_max_width,   2, object->max_width,  object->parent->math_width, minSide, maxSide, mWidth, mHeight);
        object->math_max_height = _localMath(object->format_max_height, 2, object->max_height, object->parent->math_height, minSide, maxSide, mWidth, mHeight);

        if (object->math_width < object->math_min_width) {
            object->math_width = object->math_min_width;
        } else if (object->math_max_width > 0 && object->math_width > object->math_max_width) {
            object->math_width = object->math_max_width;
        }

        if (object->math_height < object->math_min_height) {
            object->math_height = object->math_min_height;
        } else if (object->math_max_height > 0 && object->math_height > object->math_max_height) {
            object->math_height = object->math_max_height;
        }

        if (object->centering) {
            localMathX -= object->math_width / 2;
            localMathY -= object->math_height / 2;
            object->math_x = localMathX;
            object->math_y = localMathY;
        }

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

        object->math_natural_width = object->math_width;
        object->math_natural_height = object->math_height;

        object->math_width += object->offsetWidth;
        object->math_height += object->offsetHeight;

        object->math_x += object->offsetX + forceOffsetX;
        object->math_y += object->offsetY + forceOffsetY;

        object->processing = _checkIntersection(object->root->math_x, object->root->math_y, object->root->math_width, object->root->math_height, object);

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
                    object->pressed = true;

                    if (object->resizable > 0) {
                        tsgl_pos lx = x - object->math_x;
                        tsgl_pos ly = y - object->math_y;

                        if (ly < object->resizable) { //top
                            object->tActionType = 1;
                            object->needDraw = true;
                        } else if (lx < object->resizable) { //left
                            object->tActionType = 2;
                            object->needDraw = true;
                        } else if (object->math_height - ly - 1 < object->resizable) { //bottom
                            object->tActionType = 3;
                            object->needDraw = true;
                        } else if (object->math_width - lx - 1 < object->resizable) { //right
                            object->tActionType = 4;
                            object->needDraw = true;
                        } else {
                            object->tActionType = 0;
                        }
                    } else {
                        object->tActionType = 0;
                    }

                    object->tdx = object->offsetX;
                    object->tdy = object->offsetY;
                    object->tdw = object->offsetWidth;
                    object->tdh = object->offsetHeight;
                    _toUpLevel(object);
                }
                break;

            case tsgl_gui_drag:
                if (object->pressed) {
                    if (x != object->tx || y != object->ty) {
                        if (object->draggable) {
                            tsgl_pos selX = x - object->tpx;
                            tsgl_pos selY = y - object->tpy;

                            object->old_math_x = object->math_x;
                            object->old_math_y = object->math_y;
                            object->old_math_width = object->math_width;
                            object->old_math_height = object->math_height;

                            tsgl_pos minOffsetWidth = object->math_min_width - object->math_natural_width;
                            tsgl_pos maxOffsetWidth = object->math_max_width - object->math_natural_width;
                            tsgl_pos minOffsetHeight = object->math_min_height - object->math_natural_height;
                            tsgl_pos maxOffsetHeight = object->math_max_height - object->math_natural_height;

                            switch (object->tActionType) {
                                case 1 : {
                                    tsgl_pos minSel = object->offsetHeight - maxOffsetHeight;
                                    tsgl_pos maxSel = object->offsetHeight - minOffsetHeight;
                                    if (selY > maxSel) {
                                        selY = maxSel;
                                    } else if (selY < minSel) {
                                        selY = minSel;
                                    }
                                    object->offsetY = object->tdy + selY;
                                    object->offsetHeight = object->tdh - selY;
                                    break;
                                }

                                case 2:
                                    tsgl_pos minSel = object->offsetWidth - maxOffsetWidth;
                                    tsgl_pos maxSel = object->offsetWidth - minOffsetWidth;
                                    if (selX > maxSel) {
                                        selX = maxSel;
                                    } else if (selX < minSel) {
                                        selX = minSel;
                                    }
                                    object->offsetX = object->tdx + selX;
                                    object->offsetWidth = object->tdw - selX;
                                    break;

                                case 3:
                                    object->offsetHeight = object->tdh + selY;
                                    if (object->offsetHeight < minOffsetHeight) {
                                        object->offsetHeight = minOffsetHeight;
                                    } else if (object->offsetHeight > maxOffsetHeight) {
                                        object->offsetHeight = maxOffsetHeight;
                                    }
                                    break;

                                case 4:
                                    object->offsetWidth = object->tdw + selX;
                                    if (object->offsetWidth < minOffsetWidth) {
                                        object->offsetWidth = minOffsetWidth;
                                    } else if (object->offsetWidth > maxOffsetWidth) {
                                        object->offsetWidth = maxOffsetWidth;
                                    }
                                    break;
                                
                                default:
                                    object->offsetX = object->tdx + selX;
                                    object->offsetY = object->tdy + selY;
                                    break;
                            }

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
                if (object->tActionType > 0) {
                    object->tActionType = 0;
                    object->needDraw = true;
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
        if (child != child2 && !child2->drawLater && !child2->drawLaterLater && !child->localMovent && _checkIntersection(child->math_x, child->math_y, child->math_width, child->math_height, child2)) {
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
                TSGL_GUI_DRAW(object, fill, object->old_math_x, object->old_math_y, object->old_math_width, object->old_math_height, object->parent->color);
            }
        }
        object->localMovent = false;

        object->needDraw = false;
        float delta = object->animationTarget - object->animationState;
        if (delta != 0) {
            bool animEnd;
            float stateDelta;
            bool lockMul = false;
            if (object->animationSpeed == 0) {
                stateDelta = 1;
                lockMul = true;
            } else {
                stateDelta = dt / object->animationSpeed;
            }
            if (object->oldAnimationTarget != object->animationTarget) { //gives a small boost to the animation at the very beginning so that the UI does not seem slow
                stateDelta = object->animationBaseDelta;
            }
            if (delta > 0) {
                if (object->animationSpeedUpMul != 0 && !lockMul) {
                    object->animationState += stateDelta * object->animationSpeedUpMul;
                } else {
                    object->animationState += stateDelta;
                }
                animEnd = object->animationState > object->animationTarget;
            } else {
                if (object->animationSpeedDownMul != 0 && !lockMul) {
                    object->animationState -= stateDelta * object->animationSpeedDownMul;
                } else {
                    object->animationState -= stateDelta;
                }
                animEnd = object->animationState < object->animationTarget;
            }
            if (animEnd) {
                object->animationState = object->animationTarget;
            } else {
                object->needDraw = true;
            }
        } else {
            object->animationState = object->animationTarget;
        }
        object->oldAnimationTarget = object->animationTarget;

        if (!object->color.invalid) {
            TSGL_GUI_DRAW(object, fill, object->math_x, object->math_y, object->math_width, object->math_height, object->color);
        } else if (object->draw_callback != NULL) {
            if (object->fillParentSize && object->parent != NULL && !object->parent->color.invalid) {
                TSGL_GUI_DRAW(object, fill, object->math_x, object->math_y, object->math_width, object->math_height, object->parent->color);
            }
            object->draw_callback(object);
        }

        tsgl_rawcolor selectorBarColor = tsgl_color_raw(TSGL_RED, object->colormode);
        switch (object->tActionType) {
            case 1:
                TSGL_GUI_DRAW(object, fill, object->math_x, object->math_y, object->math_width, object->resizable, selectorBarColor);
                break;

            case 2:
                TSGL_GUI_DRAW(object, fill, object->math_x, object->math_y, object->resizable, object->math_height, selectorBarColor);
                break;

            case 3:
                TSGL_GUI_DRAW(object, fill, object->math_x, (object->math_y + object->math_height) - object->resizable, object->math_width, object->resizable, selectorBarColor);
                break;

            case 4:
                TSGL_GUI_DRAW(object, fill, (object->math_x + object->math_width) - object->resizable, object->math_y, object->resizable, object->math_height, selectorBarColor);
                break;
        }

        anyDraw = true;
    } else if (object->children != NULL) {
        for (size_t i = 0; i < object->childrenCount; i++) {
            tsgl_gui* child = object->children[i];
            if (child->localMovent) {
                child->drawLaterLater = true;
                for (size_t i = 0; i < object->childrenCount; i++) {
                    tsgl_gui* child2 = object->children[i];
                    if (child != child2 && _checkIntersection(child->old_math_x, child->old_math_y, child->old_math_width, child->old_math_height, child2)) {
                        child2->drawLater = true;
                        child2->needDraw = false;
                        anyDrawLater = true;
                        _recursionDrawLater(object, child2, i);
                    }
                }
            } else if (child->needDraw && !child->draggable) {
                for (size_t i = 0; i < object->childrenCount; i++) {
                    tsgl_gui* child2 = object->children[i];
                    if (child != child2 && _checkIntersection(child->math_x, child->math_y, child->math_width, child->math_height, child2)) {
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
    tsgl_gui* gui = tsgl_gui_createRoot_displayZone(display, colormode, 0, 0, display->width, display->height);
    gui->leaky_walls = true;
    return gui;
}

tsgl_gui* tsgl_gui_createRoot_buffer(tsgl_display* display, tsgl_framebuffer* framebuffer) {
    tsgl_gui* gui = tsgl_gui_createRoot_bufferZone(display, framebuffer, 0, 0, framebuffer->width, framebuffer->height);
    gui->leaky_walls = true;
    return gui;
}

tsgl_gui* tsgl_gui_createRoot_displayZone(tsgl_display* display, tsgl_colormode colormode, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    tsgl_gui* gui = _createRoot(display, false, x, y, width, height);
    gui->colormode = colormode;
    gui->display = display;
    gui->color = display->black;
    return gui;
}

tsgl_gui* tsgl_gui_createRoot_bufferZone(tsgl_display* display, tsgl_framebuffer* framebuffer, tsgl_pos x, tsgl_pos y, tsgl_pos width, tsgl_pos height) {
    tsgl_gui* gui = _createRoot(framebuffer, true, x, y, width, height);
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
    newObject->animationSpeed = 0.25;
    newObject->animationBaseDelta = 0.1;
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


void tsgl_gui_setAllFormat(tsgl_gui* object, tsgl_gui_paramFormat format) {
    tsgl_gui_setPosFormat(object, format);
    tsgl_gui_setScaleFormat(object, format);
    tsgl_gui_setMinFormat(object, format);
    tsgl_gui_setMaxFormat(object, format);
}

void tsgl_gui_setPosFormat(tsgl_gui* object, tsgl_gui_paramFormat format) {
    object->format_x = format;
    object->format_y = format;
}

void tsgl_gui_setScaleFormat(tsgl_gui* object, tsgl_gui_paramFormat format) {
    object->format_width = format;
    object->format_height = format;
}

void tsgl_gui_setMinFormat(tsgl_gui* object, tsgl_gui_paramFormat format) {
    object->format_min_width = format;
    object->format_min_height = format;
}

void tsgl_gui_setMaxFormat(tsgl_gui* object, tsgl_gui_paramFormat format) {
    object->format_max_width = format;
    object->format_max_height = format;
}

void tsgl_gui_setWidthMinMaxFormat(tsgl_gui* object, tsgl_gui_paramFormat format) {
    object->format_min_width = format;
    object->format_max_width = format;
}

void tsgl_gui_setHeightMinMaxFormat(tsgl_gui* object, tsgl_gui_paramFormat format) {
    object->format_min_height = format;
    object->format_max_height = format;
}



void tsgl_gui_processClick(tsgl_gui* obj, tsgl_pos x, tsgl_pos y, tsgl_gui_event clickType) {
    _event(obj, obj->math_x + x, obj->math_y + y, clickType);
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
    bool needSend;
    if (benchmark != NULL) {
        needSend = _draw(root, false, (benchmark->renderingTime + benchmark->sendTime) / 1000.0 / 1000.0);
    } else {
        //if the benchmark has not been transmitted, it is assumed that the rendering is running at a frequency of 10 FPS. the animation speed can float a lot
        needSend = _draw(root, false, 0.1);
    }
    if (benchmark != NULL) tsgl_benchmark_endRendering(benchmark);
    if (needSend && root->buffered) {
        if (benchmark != NULL) tsgl_benchmark_startSend(benchmark);
        if (asyncFramebuffer != NULL) {
            tsgl_display_asyncCopySend(root->display, root->target, asyncFramebuffer);
        } else {
            tsgl_display_send(root->display, root->target);
        }
        if (benchmark != NULL) tsgl_benchmark_endSend(benchmark);
    } else if (benchmark != NULL) {
        tsgl_benchmark_noSend(benchmark);
    }
}