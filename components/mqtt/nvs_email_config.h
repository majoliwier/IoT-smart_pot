#ifndef NVS_EMAIL_CONFIG_H
#define NVS_EMAIL_CONFIG_H

#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#define EMAIL_STORAGE_NAMESPACE "email_config"
#define EMAIL_KEY "email_address"

bool save_email_address(const char* email) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(EMAIL_STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error opening NVS handle for email: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_set_str(nvs_handle, EMAIL_KEY, email);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        ESP_LOGE("NVS", "Error writing email: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    return err == ESP_OK;
}

bool load_email_address(char* email, size_t max_len) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(EMAIL_STORAGE_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error opening NVS handle for email: %s", esp_err_to_name(err));
        return false;
    }

    size_t required_size;
    err = nvs_get_str(nvs_handle, EMAIL_KEY, NULL, &required_size);
    if (err != ESP_OK || required_size > max_len) {
        nvs_close(nvs_handle);
        return false;
    }

    err = nvs_get_str(nvs_handle, EMAIL_KEY, email, &required_size);
    nvs_close(nvs_handle);
    return err == ESP_OK;
}

#endif
