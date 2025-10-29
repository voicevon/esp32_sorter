#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "HELLO_WORLD";

void app_main(void)
{
    ESP_LOGI(TAG, "Hello World!");
    ESP_LOGI(TAG, "ESP32 Chip revision: %d", esp_get_chip_revision());
    
    // 打印一些系统信息
    printf("Hello from ESP32!\n");
    printf("CPU frequency: %d MHz\n", get_cpu_freq_mhz());
    
    // 循环打印Hello World
    while (1) {
        ESP_LOGI(TAG, "Hello World Loop");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}