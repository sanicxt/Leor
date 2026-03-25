#include "leor/application.hpp"

#include "esp_ota_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void) {
    static const char* kTag = "leor_main";
    static leor::Application app;

    ESP_ERROR_CHECK(app.start());
    ESP_LOGI(kTag, "Leor ESP-IDF skeleton started");

    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
        if (state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(kTag, "New OTA booted successfully. Marking valid and canceling rollback.");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(33); // ~30 FPS (33ms)

    while (true) {
        app.tick();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
