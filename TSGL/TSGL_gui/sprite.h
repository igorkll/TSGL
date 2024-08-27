#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_framebuffer.h"

typedef struct {
    tsgl_sprite* sprite;
    bool autoscale;
} tsgl_gui_spriteData;

tsgl_gui* tsgl_gui_addSprite(tsgl_gui* gui, tsgl_sprite* sprite, bool autoscale);
void tsgl_gui_sprite_setParams(tsgl_gui* self, tsgl_sprite* sprite, bool autoscale);