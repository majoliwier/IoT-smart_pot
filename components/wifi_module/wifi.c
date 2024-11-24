#include "wifi.h"

const char *ssid = "Iphoen";
const char *pass = "12345678";

#define LED_PIN GPIO_NUM_2

static EventGroupHandle_t s_wifi_event_group;

static const char *TAGwifi = "wifi station";

volatile bool is_connected = false;

static TaskHandle_t s_blink_task_handle = NULL;

bool wifi_is_connected()
{
    return is_connected;
}

static void led_init(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);
}

static void blink_task(void *pvParameter)
{
    while (1) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        is_connected = false;

        if (is_connected) {
            ESP_LOGI(TAGwifi, "Rozłączono z AP, próba ponownego połączenia...");
        } else {
            ESP_LOGI(TAGwifi, "Nie udało się połączyć z AP, próba ponownego połączenia...");
        }

        esp_wifi_connect();

        if (s_blink_task_handle == NULL) {
            xTaskCreate(blink_task, "blink_task", 1024, NULL, tskIDLE_PRIORITY, &s_blink_task_handle);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        is_connected = true;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAGwifi, "Otrzymano IP: " IPSTR, IP2STR(&event->ip_info.ip));

        if (s_blink_task_handle != NULL) {
            vTaskDelete(s_blink_task_handle);
            s_blink_task_handle = NULL;
            gpio_set_level(LED_PIN, 0);
        }

    }
}

static void wifi_task(void *pvParameters)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    led_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAGwifi, "ESP_WIFI_MODE_STA");
    ESP_LOGI(TAGwifi, "wifi_init_sta zakończone.");
    ESP_LOGI(TAGwifi, "Połączenie z AP SSID:%s password:%s", ssid, pass);

    vTaskDelete(NULL);
}