#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "../components/bh1750/bh1750.h"
#include "../components/dht11/dht11.h"
#include "../components/gatt_server/gatt_server.h"
#include "../components/soil_moisture/soil_moisture_sensor.h"
#include "../components/mqtt/nvs_email_config.h"
#include <time.h>
#include "cJSON.h"


//mac address bluetooth

// mosquitto_pub -h 192.168.10.169 -t "user123/device002/" -m "Test Subscriber" -u user123 -P user123

// mosquitto_sub -h 192.168.10.169 -t "user123/device001/temperature" -u user123 -P user123
static const char *TAG = "MQTT";

esp_mqtt_client_handle_t client;
bool mqtt_connected = false;

char user_id[32] = {0};

char device_id[18];

int illuminance_frequency = 6000;
int humidity_frequency = 10000;
int temperature_frequency = 20000;

// Flag to restart tasks
volatile bool restart_tasks = false;

void get_current_time(char *buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

void mqtt_publish(const char *topic, const char *payload) {
    if (mqtt_connected) {
        esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
        ESP_LOGI(TAG, "Published: %s -> %s", topic, payload);
    } else {
        ESP_LOGW(TAG, "MQTT not connected, cannot publish");
        mqtt_app_stop();
    }
}



void mqtt_publish_illuminance_task(void *pvParameters) {
    while (1) {
        if (restart_tasks) vTaskDelete(NULL);  // Exit on restart signal
        float lux = bh1750_read_continuous_high_res();

        char topic[100];
        snprintf(topic, sizeof(topic), "%s/%s/illuminance", user_id, device_id);

        char payload[200];
        char timestamp[32];
        get_current_time(timestamp, sizeof(timestamp));
        snprintf(payload, sizeof(payload),  "{\"illuminance\": %.2f, \"unit\": \"lx\", \"timestamp\": \"%s\"}",
         lux,timestamp);

        mqtt_publish(topic, payload);

        vTaskDelay(illuminance_frequency / portTICK_PERIOD_MS); // 6 sekund
    }
    vTaskDelete(NULL);
}

void mqtt_publish_temperature_task(void *pvParameters) {
    while (1) {
        if (restart_tasks) vTaskDelete(NULL);  // Exit on restart signal
        int *hum_temp = DHT11_read();
        float temperature = hum_temp[1];

        char topic[100];
        snprintf(topic, sizeof(topic), "%s/%s/temperature", user_id, device_id);

        char payload[200];
        char timestamp[32];
        get_current_time(timestamp, sizeof(timestamp));
        snprintf(payload, sizeof(payload),
                 "{\"temperature\": %.2f, \"unit\": \"C\", \"timestamp\": \"%s\"}",
                 temperature, timestamp);

        mqtt_publish(topic, payload);

        vTaskDelay(temperature_frequency / portTICK_PERIOD_MS); // 20 sekund
    }
    vTaskDelete(NULL);
}

void mqtt_publish_humidity_task(void *pvParameters) {
    while (1) {
        if (restart_tasks) vTaskDelete(NULL);  // Exit on restart signal
        int *hum_temp = DHT11_read();
        float humidity = hum_temp[0];

        char topic[100];
        snprintf(topic, sizeof(topic), "%s/%s/humidity", user_id, device_id);

        char payload[200];
        char timestamp[32];
        get_current_time(timestamp, sizeof(timestamp));

        snprintf(payload, sizeof(payload),
                 "{\"humidity\": %.2f, \"unit\": \"%%\", \"timestamp\": \"%s\"}",
                 humidity, timestamp);

        mqtt_publish(topic, payload);

        vTaskDelay(humidity_frequency / portTICK_PERIOD_MS); // 10 sekund
    }
    vTaskDelete(NULL);
}

void mqtt_publish_mac() {
    char topic[100];
    snprintf(topic, sizeof(topic), "%s", user_id);
    char payload[100];
    snprintf(payload, sizeof(payload), "{\"device_mac\": \"%s\"}", device_id);
    mqtt_publish(topic,payload);
    // while (1) {
    //     float lux = bh1750_read_continuous_high_res();
    //     int *hum_temp = DHT11_read();
    //     float moisture = soil_moisture_read();

    //     char topic[100];
    //     snprintf(topic, sizeof(topic), "%s/%s/illuminance", user_id, device_id);

    //     char payload[100];
    //     snprintf(payload, sizeof(payload), "{\"illuminance\": %.2f, \"unit\": \"lx\"}", lux);

    //     mqtt_publish(topic, payload);

    //     float temperature = hum_temp[1];
    //     char topic_temperature[100];
    //     snprintf(topic_temperature, sizeof(topic_temperature), "%s/%s/temperature", user_id, device_id);

    //     char payload_temperature[100];
    //     snprintf(payload_temperature, sizeof(payload_temperature), "{\"temperature\": %.2f, \"unit\": \"C\"}", temperature);

    //     mqtt_publish(topic_temperature, payload_temperature);

    //     float humidity = hum_temp[0];
    //     char topic_humidity[100];
    //     snprintf(topic_humidity, sizeof(topic_humidity), "%s/%s/humidity", user_id, device_id);

    //     char payload_humidity[100];
    //     snprintf(payload_humidity, sizeof(payload_humidity), "{\"humidity\": %.2f, \"unit\": \"%%\"}", humidity);

    //     mqtt_publish(topic_humidity, payload_humidity);

    //     vTaskDelay(6000 / portTICK_PERIOD_MS);
    // }
        // vTaskDelete(NULL);

}

// Restart tasks based on updated frequencies
void restart_publishing_tasks() {
    restart_tasks = false;
    xTaskCreate(mqtt_publish_illuminance_task, "mqtt_publish_illuminance_task", 4096, NULL, 5, NULL);
    xTaskCreate(mqtt_publish_temperature_task, "mqtt_publish_temperature_task", 4096, NULL, 5, NULL);
    xTaskCreate(mqtt_publish_humidity_task, "mqtt_publish_humidity_task", 4096, NULL, 5, NULL);
}

void mqtt_message_handler(const char *topic, const char *payload) {
    ESP_LOGI(TAG, "MQTT HANDLER: Message received on topic: %s", topic);
    ESP_LOGI(TAG, "Payload: %s", payload);

    
    cJSON *json = cJSON_Parse(payload);
    if (json == NULL) {
        ESP_LOGE(TAG, "Invalid JSON format");
        return;
    }

   if(strstr(topic, "/configuration") != NULL){
        
        cJSON *illuminance_freq = cJSON_GetObjectItem(json, "measurement_frequency_illuminance");
        cJSON *humidity_freq = cJSON_GetObjectItem(json, "measurement_frequency_humidity");
        cJSON *temperature_freq = cJSON_GetObjectItem(json, "measurement_frequency_temperature");

        restart_tasks = true;  // Signal tasks to restart
        if (illuminance_freq && illuminance_freq->valueint > 0) {
            illuminance_frequency = illuminance_freq->valueint;
            ESP_LOGI(TAG, "Updated illuminance frequency: %d ms", illuminance_frequency);
        }
        if (humidity_freq && humidity_freq->valueint > 0) {
            humidity_frequency = humidity_freq->valueint;
            ESP_LOGI(TAG, "Updated humidity frequency: %d ms", humidity_frequency);
        }
        if (temperature_freq && temperature_freq->valueint > 0) {
            temperature_frequency = temperature_freq->valueint;
            ESP_LOGI(TAG, "Updated temperature frequency: %d ms", temperature_frequency);
        }

        // restart_tasks = true;  // Signal tasks to restart
        
        cJSON_Delete(json);
        
        restart_tasks = false;
    } else if (strstr(topic, "/command") != NULL) {
        if (cJSON_IsObject(json)) {
        cJSON *action = cJSON_GetObjectItem(json, "action");
        if (cJSON_IsString(action)) {
            if (strcmp(action->valuestring, "turn_on_light") == 0) {
                // Turn on the light
                ESP_LOGI(TAG, "Action: Turning on light %s", action->valuestring);
            } else {
                ESP_LOGW(TAG, "Action: Turning on light %s", action->valuestring);
            }
        }
        }

        cJSON_Delete(json);

    } else {
        ESP_LOGE(TAG, "MQTT ERROR SUBSCRIBTION HANDLER");
    }
}



static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            mqtt_connected = true;

            char subscribe_topic[100];
            snprintf(subscribe_topic, sizeof(subscribe_topic), "%s/%s/configuration", user_id, device_id);
            esp_mqtt_client_subscribe(client, subscribe_topic, 0);
            ESP_LOGI(TAG, "Subscribed to configuration topic: %s", subscribe_topic);

            char subscribe_command[100];
            snprintf(subscribe_command, sizeof(subscribe_command),"%s/%s/command", user_id, device_id);
            
            
            esp_mqtt_client_subscribe(client, subscribe_command, 0);
            
            ESP_LOGI(TAG, "Subscribed to command topic: %s", subscribe_command);

            //xTaskCreate(mqtt_subscribe, "mqtt_subscribe")
            //xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 5, NULL);
            mqtt_publish_mac();
            restart_publishing_tasks();  // publikowanie tempperatury, humidity,illuminance
            break;
            

            

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected. The client has aborted the connection due to being unable to read or write data, e.g., because the server is unavailable");
            mqtt_connected = false;
            vTaskDelay(pdMS_TO_TICKS(5000));
            // ESP_LOGI(TAG, "Reconnecting...");
            // esp_mqtt_client_reconnect(client);
            // esp_mqtt_client_stop(client);
            // esp_mqtt_client_start(client);

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

            // Buffers for topic and payload
            char topic[256] = {0};
            char payload[450] = {0};

            // Validate and copy the topic
            if (event->topic == NULL || event->topic_len == 0) {
                ESP_LOGE(TAG, "Received message with null or empty topic");
                return;
            }

            if (event->topic_len >= sizeof(topic)) {
                ESP_LOGE(TAG, "Topic length exceeds buffer size");
                return;
            }

            strncpy(topic, event->topic, event->topic_len);
            topic[event->topic_len] = '\0';  // Null-terminate explicitly

            // Validate and copy the payload
            if (event->data_len >= sizeof(payload)) {
                ESP_LOGE(TAG, "Payload length exceeds buffer size");
                return;
            }

            strncpy(payload, event->data, event->data_len);
            payload[event->data_len] = '\0';  // Null-terminate explicitly

            ESP_LOGI(TAG, "Topic: %s", topic);
            ESP_LOGI(TAG, "Payload: %s", payload);

            // Pass topic and payload to the handler
            mqtt_message_handler(topic, payload);
            break;
        }


        default:
            //ESP_LOGI(TAG, "Other MQTT event: %ld", (long int)event_id);
            break;
    }
}

// Inicjalizacja klienta MQTT
void mqtt_app_start(void) {
    load_email_address(user_id, sizeof(user_id));
    read_device_mac(device_id, sizeof(device_id));
    ESP_LOGI(TAG, "Device MAC: %s", device_id);

    ESP_LOGI(TAG, "Device MAC: %s", device_id);
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

void mqtt_app_stop(void){
    load_email_address(user_id, sizeof(user_id));
    printf("%s", user_id);
    if (client == NULL) {
        ESP_LOGW(TAG, "MQTT client is already destroyed");
        return;
    }
    esp_err_t stop_ret = esp_mqtt_client_stop(client);
    if (stop_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop MQTT client: %s", esp_err_to_name(stop_ret));
    } else {
        ESP_LOGI(TAG, "MQTT client stopped successfully");
       
    }

    esp_err_t destroy_ret = esp_mqtt_client_destroy(client);
    if (destroy_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to destroy MQTT client: %s", esp_err_to_name(destroy_ret));
    } else {
        ESP_LOGI(TAG, "MQTT client destroyed successfully");
    }
}