#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sdkconfig.h"

#define BOOT_BUTTON GPIO_NUM_0
#define BOOT_HOLD_TIME 3000
#define TAG_BLE_SERVER "BLE-Server"

static uint8_t notify_value = 0;
static uint16_t notify_handle;
uint8_t ble_addr_type;
volatile bool ble_enabled = false; 

void ble_app_advertise(void);

static int SSID_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    printf("SSID from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);
    return 0;
}

static int PASS_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    printf("Password from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);
    return 0;
}

static int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *text = "wifi pass";
    os_mbuf_append(ctxt->om, text, strlen(text));
    return 0;
}

static int notify_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    os_mbuf_append(ctxt->om, &notify_value, sizeof(notify_value));
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(0x180),
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(0xFEF4),
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = device_read},
         {.uuid = BLE_UUID16_DECLARE(0xDEAD),
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = SSID_write},
         {.uuid = BLE_UUID16_DECLARE(0xF00D),
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = PASS_write},
         {.uuid = BLE_UUID16_DECLARE(0xBEEF),
          .flags = BLE_GATT_CHR_F_NOTIFY,
          .access_cb = notify_read,
          .val_handle = &notify_handle},
         {0}}},
    {0}};

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT DISCONNECTED");
        if (ble_enabled) {
            ble_app_advertise();
        }
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE GAP EVENT ADV COMPLETE");
        if (ble_enabled) {
            ble_app_advertise();
        }
        break;
    default:
        break;
    }
    return 0;
}

void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

void notify_task(void *param)
{
    while (1)
    {
        if (ble_enabled) {
            notify_value = !notify_value;
            ESP_LOGI(TAG_BLE_SERVER, "Notify value: %d", notify_value);

            struct os_mbuf *om = ble_hs_mbuf_from_flat(&notify_value, sizeof(notify_value));
            if (om)
            {
                ble_gattc_notify_custom(0, notify_handle, om);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
void host_task(void *param)
{
    nimble_port_run();
}

void ble_start(void)
{
    nimble_port_init();
    ble_svc_gap_device_name_set("BLE-Server");
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    ble_hs_cfg.sync_cb = ble_app_advertise;
    nimble_port_freertos_init(host_task);
    ble_enabled = true;
    ESP_LOGI(TAG_BLE_SERVER, "BLE started.");
}

void ble_stop(void)
{
    ble_gap_adv_stop();
    nimble_port_stop();
    ble_enabled = false;
    ESP_LOGI(TAG_BLE_SERVER, "BLE stopped.");
}

void button_task(void *param)
{
    gpio_set_direction(BOOT_BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOOT_BUTTON, GPIO_PULLUP_ONLY);

    while (1)
    {
        int hold_time = 0;

        while (gpio_get_level(BOOT_BUTTON) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            hold_time += 10;

            if (hold_time >= BOOT_HOLD_TIME)
            {
                ESP_LOGI(TAG_BLE_SERVER, "BOOT button held for 3 seconds.");
                if (ble_enabled) {
                    ble_stop();
                } else {
                    ble_start();
                }
                hold_time = 0;
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
