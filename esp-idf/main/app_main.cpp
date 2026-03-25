#include "leor/application.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void) {
    static const char* kTag = "leor_main";
    static leor::Application app;

    ESP_ERROR_CHECK(app.start());
    ESP_LOGI(kTag, "Leor ESP-IDF skeleton started");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(33); // ~30 FPS (33ms)

    while (true) {
        app.tick();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
