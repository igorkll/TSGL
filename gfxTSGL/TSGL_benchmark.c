#include "TSGL.h"
#include "TSGL_benchmark.h"
#include <esp_timer.h>
#include <esp_log.h>

static const char* TAG = "TSGL_benchmark";

void tsgl_benchmark_startRendering(tsgl_benchmark* benchmark) {
    benchmark->startRenderingTime = esp_timer_get_time();
}

void tsgl_benchmark_endRendering(tsgl_benchmark* benchmark) {
    benchmark->endRenderingTime = esp_timer_get_time();
    benchmark->renderingTime = benchmark->endRenderingTime - benchmark->startRenderingTime;
}

void tsgl_benchmark_endRendering_startSend(tsgl_benchmark* benchmark) {
    tsgl_benchmark_endRendering(benchmark);
    tsgl_benchmark_startSend(benchmark);
}

void tsgl_benchmark_startSend(tsgl_benchmark* benchmark) {
    benchmark->startSendTime = esp_timer_get_time();
}

void tsgl_benchmark_endSend(tsgl_benchmark* benchmark) {
    benchmark->endSendTime = esp_timer_get_time();
    benchmark->sendTime = benchmark->endSendTime - benchmark->startSendTime;
    benchmark->totalFPS = 1.0 / ((benchmark->renderingTime + benchmark->sendTime) / 1000.0 / 1000.0);
}

void tsgl_benchmark_print(tsgl_benchmark* benchmark) {
    ESP_LOGI(TAG, "rendering time: %i", benchmark->renderingTime);
    ESP_LOGI(TAG, "send      time: %i", benchmark->sendTime);
    ESP_LOGI(TAG, "total     fps:  %.1f", benchmark->totalFPS);
}