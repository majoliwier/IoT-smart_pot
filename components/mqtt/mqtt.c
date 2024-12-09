#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "../components/bh1750/bh1750.c"

// static const char *TAG = "MQTT_EXAMPLE";

// static esp_mqtt_client_handle_t client;


// const char *brokerIp = "mqtt://192.168.10.169";


// //publikacja danych (uruchamiana w osobnym zadaniu)
// void mqtt_publish_task(void *pvParameters) {
//     while (1) {
//         char topic[50];
//         char payload[50];
//         float lux = bh1750_read();
        
//         snprintf(topic, sizeof(topic), "user123/device001/temperature");
//         snprintf(payload, sizeof(payload), "{\"value\": %.2f, \"unit\": \"C\"}", temperature);

//         // publikowanie danych do brokera MQTT
//         esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
//         ESP_LOGI(TAG, "Published: %s -> %s", topic, payload);

    
//         vTaskDelay(3000 / portTICK_PERIOD_MS);
//     }
// }

// // obsługa zdarzeń MQTT
// static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
//     esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

//     switch (event_id) {
//         case MQTT_EVENT_CONNECTED:
//             ESP_LOGI(TAG, "MQTT Connected");
//             //zadanie publikującego dane
//             xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 5, NULL);
//             break;

//         case MQTT_EVENT_DISCONNECTED:
//             ESP_LOGI(TAG, "MQTT Disconnected");
//             break;

//         case MQTT_EVENT_ERROR:
//             ESP_LOGE(TAG, "MQTT Error");
//             break;

//         default:
//             ESP_LOGI(TAG, "Other MQTT event: %ld", (long int)event_id);
//             break;
//     }
// }

// // inicjalizacja klienta MQTT
// void mqtt_app_start(void) {
//     esp_mqtt_client_config_t mqtt_cfg = {
//         .broker = {
//             .address.uri = "mqtt://192.168.10.169",
//         },
        
//     };

//     // inicjalizacja
//     client = esp_mqtt_client_init(&mqtt_cfg);

//     // rejestracja zdarzeń
//     esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

//     // start klienta MQTT
//     esp_mqtt_client_start(client);
// }


//mac address bluetooth

// mosquitto_pub -h 192.168.10.169 -t "user123/device002/" -m "Test Subscriber" -u user123 -P user123

// mosquitto_sub -h 192.168.10.169 -t "user123/device001/temperature" -u user123 -P user123
static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t client;
static bool mqtt_connected = false;

const char *user_id = "user123";


//uzyskac adres mac za pomocą BLE #TODO
const char *device_id = "F0-03-8C-B7-4A-62";

void mqtt_publish(const char *topic, const char *payload) {
    if (mqtt_connected) {
        esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
        ESP_LOGI(TAG, "Published: %s -> %s", topic, payload);
    } else {
        ESP_LOGW(TAG, "MQTT not connected, cannot publish");
    }
}

void mqtt_publish_task(void *pvParameters) {
    while (1) {
        float lux = bh1750_read();

        char topic[100];
        snprintf(topic, sizeof(topic), "%s/%s/temperature", user_id, device_id);

        char payload[100];
        snprintf(payload, sizeof(payload), "{\"value\": %.2f, \"unit\": \"C\"}", lux);

        mqtt_publish(topic, payload);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void mqtt_message_handler(const char *topic, const char *payload) {
    ESP_LOGI(TAG, "Message received on topic: %s", topic);
    ESP_LOGI(TAG, "Payload: %s", payload);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            mqtt_connected = true;

            char subscribe_topic[100];
            snprintf(subscribe_topic, sizeof(subscribe_topic), "%s/%s/", user_id, device_id);
            esp_mqtt_client_subscribe(client, subscribe_topic, 0);
            ESP_LOGI(TAG, "Subscribed to topic: %s", subscribe_topic);

            xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 5, NULL);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected");
            mqtt_connected = false;
            ESP_LOGI(TAG, "Reconnecting...");
            esp_mqtt_client_reconnect(client);

            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT Error occurred");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
                ESP_LOGE(TAG, "TLS Error: 0x%x", event->error_handle->esp_tls_last_esp_err);
            } else {
                ESP_LOGE(TAG, "Error type: 0x%x", event->error_handle->error_type);
            }
            break;

        case MQTT_EVENT_DATA: {
            ESP_LOGI(TAG, "MQTT Event Data Received");
            char topic[100];
            char payload[100];

            snprintf(topic, event->topic_len + 1, "%.*s", event->topic_len, event->topic);
            snprintf(payload, event->data_len + 1, "%.*s", event->data_len, event->data);

            mqtt_message_handler(topic, payload);
            break;
        }

        default:
            ESP_LOGI(TAG, "Other MQTT event: %ld", (long int)event_id);
            break;
    }
}

// Inicjalizacja klienta MQTT
void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            // .address.uri = "mqtt://192.168.10.169",
            // .address.uri = "mqtt://172.20.10.8",
            .address.uri = "mqtt://broker.hivemq.com",
            // .address.uri = "mqtt://172.20.10.8",
            
            
            // .address.port = 1883,
        },
    };

    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_mqtt_client_start(client);
}
