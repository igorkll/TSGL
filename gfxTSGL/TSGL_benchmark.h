#pragma once
#include "TSGL.h"

typedef struct {
    int64_t startRenderingTime;
    int64_t endRenderingTime;
    int64_t renderingTime;

    int64_t startSendTime;
    int64_t endSendTime;
    int64_t oldEndSendTime;
    int64_t sendTime;

    int64_t totalTime;

    uint64_t currentFrame;
    uint64_t _currentFrame;
    bool endSendCalled;

    float totalFPS; //this parameter is relevant only if rendering and sending are performed in the same thread, they go strictly in turn and there is nothing between them
    uint32_t realFPS; //calculates based on the number of tsgl_benchmark_endSend calls per second
} tsgl_benchmark;

void tsgl_benchmark_startRendering(tsgl_benchmark* benchmark);
void tsgl_benchmark_endRendering(tsgl_benchmark* benchmark);
void tsgl_benchmark_startSend(tsgl_benchmark* benchmark);
void tsgl_benchmark_endSend(tsgl_benchmark* benchmark);
void tsgl_benchmark_print(tsgl_benchmark* benchmark);
void tsgl_benchmark_wait(tsgl_benchmark* benchmark, float targetFPS); //waits for the right time in order not to exceed the target frequency of frames, you need to call after endSend

int tsgl_benchmark_getWait(tsgl_benchmark* benchmark, float targetFPS); //returns the time to wait so as not to refresh the screen more often than necessary to ensure the target FPS
float tsgl_benchmark_processMul(tsgl_benchmark* benchmark, float targetFPS); //calculates a multiplier for the speed of processes based on the refresh rate of the screen and the target frequency. if the real frequency is equal to the target, then the number will be 1. if the real frequency is two times less, then the number will be 2 and so on
int tsgl_benchmark_processMulInt(tsgl_benchmark* benchmark, float targetFPS); //unlike tsgl_benchmark_processMul, this method cannot return less than one