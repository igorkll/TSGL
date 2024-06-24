#pragma once
#include "TSGL.h"

typedef struct {
    int64_t startRenderingTime;
    int64_t endRenderingTime;
    int64_t renderingTime;

    int64_t startSendTime;
    int64_t endSendTime;
    int64_t sendTime;
} tsgl_benchmark;

void tsgl_benchmark_startRendering(tsgl_benchmark* benchmark);
void tsgl_benchmark_endRendering(tsgl_benchmark* benchmark);
void tsgl_benchmark_startSend(tsgl_benchmark* benchmark);
void tsgl_benchmark_endSend(tsgl_benchmark* benchmark);