#include "wifi.h"

const char *ssid = "Iphoen";
const char *pass = "12345678";

#define LED_PIN GPIO_NUM_2  

static bool is_connected = false;

bool wifi_is_connected()
{
    return is_connected;
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);  

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        printf("Łączenie z wifi...\n");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        printf("Połączono z wifi.\n");
        is_connected = true;  
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("Stracono połączenie z wifi.\n");
        is_connected = false; 
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        printf("Uzyskano adress IP.\n");
        is_connected = true;
    }
}

void wifi_connection()
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",
        }};

    strcpy((char *)wifi_configuration.sta.ssid, ssid);
    strcpy((char *)wifi_configuration.sta.password, pass);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);

    esp_wifi_start();  
    esp_wifi_connect();
    while (!wifi_is_connected())
    {
        esp_wifi_connect();
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(100 );
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(100 );
    
    }
    
    printf("Zainicjowano połączenie. SSID:%s, hasło:%s\n", ssid, pass);
}

void reconnect_wifi()
{
    if (!wifi_is_connected())
    {
        esp_wifi_connect();

        gpio_set_level(LED_PIN, 1);
        vTaskDelay(100 );
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(100 );
    }
    
}