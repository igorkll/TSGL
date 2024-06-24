#include "TSGL.h"
#include "TSGL_benchmark.h"
#include <esp_timer.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "TSGL_benchmark";

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
    benchmark->currentFrame++;
    benchmark->_currentFrame++;

    benchmark->endSendTime = esp_timer_get_time();
    benchmark->sendTime = benchmark->endSendTime - benchmark->startSendTime;
    benchmark->totalTime = benchmark->renderingTime + benchmark->sendTime;
    benchmark->totalFPS = 1.0 / (benchmark->totalTime / 1000.0 / 1000.0);
    
    if (benchmark->endSendCalled) {
        if (benchmark->endSendTime - benchmark->oldEndSendTime > 1000000) {
            benchmark->realFPS = benchmark->_currentFrame;
            benchmark->_currentFrame = 0;
            benchmark->oldEndSendTime = benchmark->endSendTime;
        }
    } else {
        benchmark->oldEndSendTime = benchmark->endSendTime;
    }
    benchmark->endSendCalled = true;
}

void tsgl_benchmark_print(tsgl_benchmark* benchmark) {
    ESP_LOGI(TAG, "------ tsgl benchmark ------");
    ESP_LOGI(TAG, "rendering time: %.3f", benchmark->renderingTime / 1000.0 / 1000.0);
    ESP_LOGI(TAG, "send      time: %.3f", benchmark->sendTime / 1000.0 / 1000.0);
    ESP_LOGI(TAG, "total     fps:  %.3f", benchmark->totalFPS);
    ESP_LOGI(TAG, "real      fps:  %li",  benchmark->realFPS);
    ESP_LOGI(TAG, "----------------------------");
}

void tsgl_benchmark_wait(tsgl_benchmark* benchmark, float targetFPS) {
    int waitTime = tsgl_benchmark_getWait(benchmark, targetFPS);
    if (waitTime > 0)
        vTaskDelay(waitTime / portTICK_PERIOD_MS);
}


int tsgl_benchmark_getWait(tsgl_benchmark* benchmark, float targetFPS) {
    int64_t targetTime = (1 / targetFPS) * 1000 * 1000;
    if (benchmark->totalTime < targetTime)
        return (targetTime - benchmark->totalTime) / 1000;
    return 0;
}

float tsgl_benchmark_processMul(tsgl_benchmark* benchmark, float targetFPS) {
    if (!benchmark->endSendCalled) return 1;
    return targetFPS / benchmark->realFPS;
}

int tsgl_benchmark_processMulInt(tsgl_benchmark* benchmark, float targetFPS) {
    float mul = tsgl_benchmark_processMul(benchmark, targetFPS);
    if (mul < 1)
        return 1;
    return mul + 0.5;
}