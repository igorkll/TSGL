#include "TSGL.h"
#include "TSGL_benchmark.h"
#include <esp_timer.h>

void tsgl_benchmark_startRendering(tsgl_benchmark* benchmark) {
    benchmark->startRenderingTime = esp_timer_get_time();
}

void tsgl_benchmark_endRendering(tsgl_benchmark* benchmark) {
    benchmark->endRenderingTime = esp_timer_get_time();
    benchmark->renderingTime = benchmark->endRenderingTime - benchmark->startRenderingTime;
}

void tsgl_benchmark_startSend(tsgl_benchmark* benchmark) {
    benchmark->startSendTime = esp_timer_get_time();
}

void tsgl_benchmark_endSend(tsgl_benchmark* benchmark) {
    benchmark->endSendTime = esp_timer_get_time();
    benchmark->sendTime = benchmark->endSendTime - benchmark->startSendTime;
}