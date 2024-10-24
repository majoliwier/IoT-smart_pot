#include "driver/gpio.h" // piny
#include "freertos/FreeRTOS.h" // delay
#include "freertos/task.h" //delay
#include <string.h> // printf
#include "esp_wifi.h" // wifi
#include "esp_log.h" // logi
#include "esp_event.h" // eventy

static void wifi_event_handler(void*, esp_event_base_t, int32_t, void*);

void wifi_connection();

void reconnect_wifi();

bool wifi_is_connected();
