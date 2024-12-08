#ifndef NVS_WIFI_CONFIG_H
#define NVS_WIFI_CONFIG_H

#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#define STORAGE_NAMESPACE "wifi_config"
#define SSID_KEY "ssid"
#define PASS_KEY "password"

bool save_wifi_credentials(const char* ssid, const char* password) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error opening NVS handle: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_set_str(nvs_handle, SSID_KEY, ssid);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        ESP_LOGE("NVS", "Error writing SSID: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_set_str(nvs_handle, PASS_KEY, password);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        ESP_LOGE("NVS", "Error writing Password: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    return err == ESP_OK;
}

bool load_wifi_credentials(char* ssid, char* password, size_t max_len) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error opening NVS handle: %s", esp_err_to_name(err));
        return false;
    }

    size_t required_size;
    err = nvs_get_str(nvs_handle, SSID_KEY, NULL, &required_size);
    if (err != ESP_OK || required_size > max_len) {
        nvs_close(nvs_handle);
        return false;
    }
    err = nvs_get_str(nvs_handle, SSID_KEY, ssid, &required_size);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return false;
    }

    err = nvs_get_str(nvs_handle, PASS_KEY, NULL, &required_size);
    if (err != ESP_OK || required_size > max_len) {
        nvs_close(nvs_handle);
        return false;
    }
    err = nvs_get_str(nvs_handle, PASS_KEY, password, &required_size);

    nvs_close(nvs_handle);
    return err == ESP_OK;
}

#endif 