#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "esp_log.h"

static const char *TAGtime  = "TIME_SYNC";

void initialize_sntp(void) {
    ESP_LOGI(TAGtime , "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");  // Możesz zmienić serwer NTP
    sntp_init();
}

void synchronize_time(void) {
    initialize_sntp();
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int max_retries = 10;

    while (timeinfo.tm_year < (2020 - 1900) && ++retry < max_retries) {
        ESP_LOGI("TIME_SYNC", "Waiting for system time to be set... (%d/%d)", retry, max_retries);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year < (2020 - 1900)) {
        ESP_LOGE("TIME_SYNC", "Failed to synchronize time");
    } else {
        ESP_LOGI("TIME_SYNC", "Time synchronized: %s", asctime(&timeinfo));
    }

    setenv("TZ", "CET-1", 1);
    tzset();

    ESP_LOGI("TIMEZONE", "Time zone set to Warsaw (CET/CEST)");
}


void obtain_time(void) {
    initialize_sntp();

    // Czekaj na synchronizację czasu
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2020 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAGtime , "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year < (2020 - 1900)) {
        ESP_LOGE(TAGtime , "Failed to obtain time");
    } else {
        ESP_LOGI(TAGtime , "Time synchronized: %s", asctime(&timeinfo));
    }
}

void set_time_zone(const char *tz) {
    setenv("TZ", tz, 1);
    tzset();
}
