#include "esp_timer.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dht11.h"

static const char *TAG_DHT11 = "DHT11";
static gpio_num_t dht_gpio;

static int waitForLevel(uint16_t timeout_us, int level) {
    int elapsed = 0;
    while (gpio_get_level(dht_gpio) == level) {
        if (++elapsed > timeout_us) return -1;
        ets_delay_us(1);
    }
    return elapsed;
}

static void sendStartSignal() {
    gpio_set_direction(dht_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_gpio, 0);
    ets_delay_us(20000);
    gpio_set_level(dht_gpio, 1);
    ets_delay_us(40);
    gpio_set_direction(dht_gpio, GPIO_MODE_INPUT);
}

static int validateCRC(uint8_t data[]) {
    return data[4] == (data[0] + data[1] + data[2] + data[3]) ? 0 : -1;
}

void DHT11_init(gpio_num_t gpio_num) {
    dht_gpio = gpio_num;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

int* DHT11_read() {
    static int result[2]; 
    static int64_t last_read_time = -2000000;
    static int last_humidity = -1;
    static int last_temperature = -1;

    if (esp_timer_get_time() - last_read_time < 2000000) {
        result[0] = last_humidity;
        result[1] = last_temperature;
        return result;
    }
    last_read_time = esp_timer_get_time();

    uint8_t data[5] = {0};
    sendStartSignal();

    if (waitForLevel(80, 0) == -1 || waitForLevel(80, 1) == -1) {
        result[0] = -1;
        result[1] = -1;
        return result;
    }

    for (int i = 0; i < 40; i++) {
        if (waitForLevel(50, 0) == -1) {
            result[0] = -1;
            result[1] = -1;
            return result;
        }
        int pulseWidth = waitForLevel(70, 1);
        if (pulseWidth == -1) {
            result[0] = -1;
            result[1] = -1;
            return result;
        }
        if (pulseWidth > 28) data[i / 8] |= (1 << (7 - (i % 8)));
    }

    if (validateCRC(data) == 0) {
        last_humidity = data[0];
        last_temperature = data[2];
        result[0] = data[0];
        result[1] = data[2];
        ESP_LOGI(TAG_DHT11, "temperature=%d\u00b0C, humidity=%d%%", data[2], data[0]);
        return result;
    } else {
        result[0] = -1;
        result[1] = -1;
        return result;
    }
}
