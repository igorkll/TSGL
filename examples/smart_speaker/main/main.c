#include "system.h"

tsgl_sound sound;

void* button1Callback(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    system_setRed(arg0 ? 255 : 0);
    if (arg0) {
        if (sound.playing) {
            tsgl_sound_stop(&sound);
        } else {
            tsgl_sound_play(&sound);
        }
    }
    return NULL;
}

void* button2Callback(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    system_setBlue(arg0 ? 255 : 0);
    //tsgl_sound_seek(&sound, -12451);
    tsgl_sound_setSpeed(&sound, arg0 ? 1.7 : 1);
    //tsgl_sound_setPause(&sound, !arg0);
    return NULL;
}

void* colorpickerCallback(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    tsgl_color* color = arg1;
    tsgl_gui* window = userArg;
    window->color = tsgl_color_raw(*color, window->colormode);
    window->needDraw = true;
    return NULL;
}

void app_main() {
    system_init();
    
    tsgl_gui* gui = tsgl_gui_createRoot_buffer(&display, &framebuffer);
    //tsgl_gui* gui = tsgl_gui_createRoot_display(&display, display.colormode);

    tsgl_gui* scene1 = tsgl_gui_addObject(gui);
    scene1->color = tsgl_color_raw(tsgl_color_fromHex(0x018db4), gui->colormode);
    
    tsgl_gui* scene2 = tsgl_gui_addObject(gui);
    scene2->color = tsgl_color_raw(tsgl_color_fromHex(0x018db4), gui->colormode);

    tsgl_gui* colorpicker = tsgl_gui_addColorpicker(scene1, (tsgl_gui_colorpickerConfig) {});
    tsgl_gui_setPosFormat(colorpicker, tsgl_gui_percent);
    tsgl_gui_setScaleFormat(colorpicker, tsgl_gui_percent);
    colorpicker->x = 0;
    colorpicker->y = 0;
    colorpicker->width = 0.7;
    colorpicker->height = 1;
    colorpicker->user_callback = colorpickerCallback;

    tsgl_gui* selScene2 = tsgl_gui_addButton(scene1, TSGL_WHITE);
    selScene2->x = framebuffer.width - 50;
    selScene2->width = 50;
    selScene2->height = 50;
    tsgl_gui_button_sceneLink(selScene2, scene2);

    tsgl_gui* window = tsgl_gui_addObject(scene2);
    tsgl_gui_setPosFormat(window, tsgl_gui_percent);
    tsgl_gui_setScaleFormat(window, tsgl_gui_percentMinSide);
    //tsgl_gui_setMinFormat(window, tsgl_gui_percentMinSide);
    //tsgl_gui_setMaxFormat(window, tsgl_gui_percentMinSide);
    window->color = tsgl_color_raw(TSGL_BLACK, window->colormode);
    window->resizable = 16;
    window->draggable = true;
    window->centering = true;
    window->x = 0.5;
    window->y = 0.5;
    window->width = 0.8;
    window->height = 0.8;
    window->min_width = 128;
    window->min_height = 128;
    window->max_width = 256;
    window->max_height = 256;

    colorpicker->userArg = window;

    tsgl_gui* button = tsgl_gui_addButton(window, TSGL_RED);
    tsgl_gui_setAllFormat(button, tsgl_gui_percentMaxSide);
    tsgl_gui_button_setText(button, TSGL_WHITE, 10, "BUTT 1", false);
    button->x = 0.1;
    button->y = 0.1;
    button->width = 0.4;
    button->height = 0.4;
    button->user_callback = button1Callback;

    tsgl_gui* button2 = tsgl_gui_addButton(window, TSGL_BLUE);
    tsgl_gui_button_setText(button2, TSGL_WHITE, 10, "BUTT 2", false);
    tsgl_gui_setAllFormat(button2, tsgl_gui_percentMaxSide);
    button2->x = 0.5;
    button2->y = 0.1;
    button2->width = 0.4;
    button2->height = 0.4;
    button2->user_callback = button2Callback;

    tsgl_framebuffer spriteFramebuffer;
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&spriteFramebuffer, display.colormode, 64, 32, TSGL_SPIRAM));
    tsgl_framebuffer_clear(&spriteFramebuffer, tsgl_color_raw(TSGL_ORANGE, spriteFramebuffer.colormode));
    for (int i = 0; i < 50; i++) {
        tsgl_framebuffer_set(&spriteFramebuffer, i, i, tsgl_color_raw(TSGL_RED, spriteFramebuffer.colormode));
    }
    tsgl_framebuffer_fill(&spriteFramebuffer, 4, 4, 16, 16, tsgl_color_raw(TSGL_GREEN, spriteFramebuffer.colormode));

    tsgl_sprite sprite = {
        .sprite = &spriteFramebuffer
    };

    tsgl_gui* spriteobj = tsgl_gui_addSprite(window, &sprite, true);
    tsgl_gui_setAllFormat(spriteobj, tsgl_gui_percent);
    spriteobj->x = 0;
    spriteobj->y = 0.8;
    spriteobj->width = 1;
    spriteobj->height = 0.2;

    tsgl_keyboard_bindToGui(&keyboard, 'A', button);
    tsgl_keyboard_bindToGui(&keyboard, 'B', button2);

    ESP_ERROR_CHECK_WITHOUT_ABORT(tsgl_sound_load_pcm(&sound, 16000, TSGL_SPIRAM, "/storage/test.wav", 8000, 1, 1, tsgl_sound_pcm_unsigned));
    tsgl_sound_output* outputs[] = {tsgl_sound_newDacOutput(DAC_CHAN_0), tsgl_sound_newDacOutput(DAC_CHAN_1)};
    tsgl_sound_setOutputs(&sound, outputs, 2, true);
    tsgl_sound_setLoop(&sound, true);
    tsgl_sound_setVolume(&sound, 1.5);

    /*
    tsgl_sound sound1;
    tsgl_sound sound2;
    tsgl_sound_instance(&sound1, &sound);
    tsgl_sound_instance(&sound2, &sound);

    tsgl_sound_play(&sound1);
    vTaskDelay(100);
    tsgl_sound_play(&sound2);
    */

    tsgl_gui_select(scene1);

    int i = 0;
    while (true) {
        tsgl_touchscreen_point points[2] = {
            {
                .x = 80,
                .y = 80,
                .z = 1
            }
        };
        tsgl_touchscreen_imitateClicks(&touchscreen, points, i % 2);

        tsgl_keyboard_readAll(&keyboard);
        tsgl_gui_processTouchscreen(gui, &touchscreen);
        tsgl_gui_processGui(gui, &framebuffer2, &benchmark);
        //tsgl_benchmark_print(&benchmark);
    }
}