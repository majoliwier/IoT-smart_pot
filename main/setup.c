#include "setup.h"

void setup(){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    bh1750_init();
    DHT11_init(GPIO_NUM_4);
    soil_moisture_init();
    xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
    xTaskCreate(notify_task, "notify_task", 4096, NULL, 5, NULL);
    xTaskCreate(&wifi_task, "wifi_task", 4096, NULL, 5, NULL);

}