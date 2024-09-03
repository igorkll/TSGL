#include "TSGL.h"
#include "TSGL_benchmark.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

static const char* TAG = "TSGL_benchmark";

void tsgl_benchmark_reset(tsgl_benchmark* benchmark) {
    memset(benchmark, 0, sizeof(tsgl_benchmark));
}

void tsgl_benchmark_startRendering(tsgl_benchmark* benchmark) {
    benchmark->startRenderingTime = tsgl_time();
}

void tsgl_benchmark_endRendering(tsgl_benchmark* benchmark) {
    benchmark->endRenderingTime = tsgl_time();
    benchmark->renderingTime = benchmark->endRenderingTime - benchmark->startRenderingTime;
    benchmark->renderingFlag = true;
}

void tsgl_benchmark_noRendering(tsgl_benchmark* benchmark) {
    benchmark->renderingFlag = false;
    benchmark->renderingTime = 0;
}

void tsgl_benchmark_startSend(tsgl_benchmark* benchmark) {
    benchmark->startSendTime = tsgl_time();
}

void tsgl_benchmark_endSend(tsgl_benchmark* benchmark) {
    benchmark->endSendTime = tsgl_time();
    benchmark->sendTime = benchmark->endSendTime - benchmark->startSendTime;

    benchmark->currentFrame++;
    if (benchmark->endSendCalled) {
        if (benchmark->endSendTime - benchmark->oldEndSendTime > 1000) {
            benchmark->realFPS = benchmark->currentFrame;
            benchmark->realFPSexists = true;
            benchmark->currentFrame = 0;
            benchmark->oldEndSendTime = benchmark->endSendTime;
        }
    } else {
        benchmark->oldEndSendTime = benchmark->endSendTime;
    }
    benchmark->endSendCalled = true;
    benchmark->sendFlag = true;
}

void tsgl_benchmark_noSend(tsgl_benchmark* benchmark) {
    benchmark->sendFlag = false;
    benchmark->sendTime = 0;
    benchmark->realFPSexists = false;
}

void tsgl_benchmark_print(tsgl_benchmark* benchmark) {
    benchmark->totalTime = 0;
    if (benchmark->renderingFlag) benchmark->totalTime += benchmark->renderingTime;
    if (benchmark->sendFlag) benchmark->totalTime += benchmark->sendTime;
    benchmark->totalFPS = 1.0 / (benchmark->totalTime / 1000.0);

    ESP_LOGI(TAG, "------ tsgl benchmark ------");
    if (benchmark->renderingFlag)
        ESP_LOGI(TAG, "rendering time: %.3f", benchmark->renderingTime / 1000.0);
    if (benchmark->sendFlag)
        ESP_LOGI(TAG, "send      time: %.3f", benchmark->sendTime / 1000.0);
    if (benchmark->totalTime > 0)
        ESP_LOGI(TAG, "total     fps:  %.3f", benchmark->totalFPS);
    if (benchmark->realFPSexists)
        ESP_LOGI(TAG, "real      fps:  %li",  benchmark->realFPS);
    ESP_LOGI(TAG, "----------------------------");
}

void tsgl_benchmark_wait(tsgl_benchmark* benchmark, float targetFPS) {
    time_t waitTime = tsgl_benchmark_getWait(benchmark, targetFPS);
    if (waitTime > 0)
        vTaskDelay(waitTime / portTICK_PERIOD_MS);
}


time_t tsgl_benchmark_getWait(tsgl_benchmark* benchmark, float targetFPS) {
    int64_t targetTime = (1 / targetFPS) * 1000;
    if (benchmark->totalTime < targetTime)
        return targetTime - benchmark->totalTime;
    return 0;
}

float tsgl_benchmark_processMul(tsgl_benchmark* benchmark, float targetFPS) {
    if (!benchmark->endSendCalled) return 1;
    if (benchmark->realFPS != 0) return targetFPS / benchmark->realFPS;
    if (benchmark->totalFPS != 0) return targetFPS / benchmark->totalFPS;
    return 1;
}

int tsgl_benchmark_processMulInt(tsgl_benchmark* benchmark, float targetFPS) {
    float mul = tsgl_benchmark_processMul(benchmark, targetFPS);
    if (mul < 1)
        return 1;
    return mul + 0.5;
}