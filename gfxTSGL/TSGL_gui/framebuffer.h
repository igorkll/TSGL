#pragma once
#include "../TSGL.h"
#include "../TSGL_gui.h"
#include "../TSGL_framebuffer.h"

typedef struct {
    uint8_t rotation;
    tsgl_framebuffer* framebuffer;
    tsgl_rawcolor transparentColor;
} tsgl_gui_framebuffer_settings;

tsgl_gui* tsgl_gui_addFramebuffer(tsgl_gui* gui, uint8_t rotation, tsgl_framebuffer* framebuffer, tsgl_rawcolor transparentColor);