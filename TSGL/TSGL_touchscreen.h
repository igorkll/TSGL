#pragma once
#include "TSGL.h"
#include "TSGL_i2c.h"
#include <esp_err.h>
#include <driver/gpio.h>

typedef enum {
    tsgl_touchscreen_capacitive_empty, //you can create an "empty" touch screen and simulate its clicks from the code yourself(tsgl_touchscreen_imitateClicks)
    tsgl_touchscreen_capacitive_ft6336u
} tsgl_touchscreen_type;

typedef struct {
    tsgl_pos x;
    tsgl_pos y;
    float z;
} tsgl_touchscreen_point;

typedef struct {
    tsgl_touchscreen_type type;
    void* ts;

    tsgl_touchscreen_point* imitateClicks;
    uint8_t imitateClicksCount;
    uint8_t realClicksCount;

    //universal calibration, suitable also for capacitive screens, but often in cases with capacitive screens, the controller can be calibrated
    //if you have a resistive screen, then use a special structure to calibrate it, and in these parameters specify only two mandatory ones (width, height in rotation 0)
    //the fields are listed here in the order of applying
    bool flipXY; //swaps X and Y
    float mulX; // if 0, it is counted as 1
    float mulY;
    float offsetX;
    float offsetY;
    tsgl_pos width; //required parameters. the width and height of the touch screen in pixels. it is required to specify even on a resistive screen
    tsgl_pos height;
    bool flipX;
    bool flipY;
    uint8_t rotation;
} tsgl_touchscreen;

esp_err_t tsgl_touchscreen_empty(tsgl_touchscreen* touchscreen);
esp_err_t tsgl_touchscreen_ft6336u(tsgl_touchscreen* touchscreen, i2c_port_t host, uint8_t address, gpio_num_t rst);
void tsgl_touchscreen_free(tsgl_touchscreen* touchscreen);

//sets an array of simulated taps on touch screen that will be processed on a par with real taps
//the array will be copied into the structure of the touch screen and you do not need to maintain its existence
//to stop simulating touches, pass an empty array(null) and 0 to the number
//please note that the touchscreen calibration fields do not affect the simulation of clicks. also, the rotation field does not affect the operation of the click simulation
void tsgl_touchscreen_imitateClicks(tsgl_touchscreen* touchscreen, tsgl_touchscreen_point* imitateClicks, uint8_t imitateClicksCount);
//calling this method before getting positions is MANDATORY
uint8_t tsgl_touchscreen_touchCount(tsgl_touchscreen* touchscreen);
tsgl_touchscreen_point tsgl_touchscreen_getPoint(tsgl_touchscreen* touchscreen, uint8_t index);