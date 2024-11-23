#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"

static const char *TAG = "MQTT_EXAMPLE";

static esp_mqtt_client_handle_t client;


const char *brokerIp = "mqtt://192.168.10.169";


//publikacja danych (uruchamiana w osobnym zadaniu)
void mqtt_publish_task(void *pvParameters) {
    float temperature = 0.0;
    while (1) {
        
        temperature += 0.1;
        char topic[50];
        char payload[50];

        snprintf(topic, sizeof(topic), "user123/device001/temperature");
        snprintf(payload, sizeof(payload), "{\"value\": %.2f, \"unit\": \"C\"}", temperature);

        // publikowanie danych do brokera MQTT
        esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
        ESP_LOGI(TAG, "Published: %s -> %s", topic, payload);

    
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// obsługa zdarzeń MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            //zadanie publikującego dane
            xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 5, NULL);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT Error");
            break;

        default:
            ESP_LOGI(TAG, "Other MQTT event: %ld", (long int)event_id);
            break;
    }
}

// inicjalizacja klienta MQTT
void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = "mqtt://192.168.10.169",
        },
        
    };

    // inicjalizacja
    client = esp_mqtt_client_init(&mqtt_cfg);

    // rejestracja zdarzeń
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    // start klienta MQTT
    esp_mqtt_client_start(client);
}