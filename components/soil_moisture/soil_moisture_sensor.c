#include "driver/adc.h"
#include "esp_log.h"
#include "soil_moisture_sensor.h"


#define SOIL_MOISTURE_ADC_CHANNEL ADC_CHANNEL_6
#define TAG_SOIL "SOIL_MOISTURE_SENSOR"


void soil_moisture_init(void) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(SOIL_MOISTURE_ADC_CHANNEL, ADC_ATTEN_DB_11);
}

float soil_moisture_read(void) {
    int raw_value = adc1_get_raw(SOIL_MOISTURE_ADC_CHANNEL);
    float moisture_percentage = (1 - (float)raw_value / 4095) * 100;

    ESP_LOGI(TAG_SOIL, "adc_raw: %d, moisture procentage: %.0f%%", raw_value, moisture_percentage);
    return moisture_percentage;

}