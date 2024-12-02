#include "gatt_client.h"

static const char *TAG_ble_client = "BLE-Client";
uint8_t ble_addr_type;
static bool is_connected_ble = false;
static bool is_scanning = false;

void ble_scan(void);

static int characteristic_read(uint16_t conn_handle, const struct ble_gatt_error *error, 
                                   struct ble_gatt_attr *attr, void *arg)
{
    if (error->status == 0) {
        printf("Read characteristic completed; status=%d conn_handle=%d\n", 
                error->status, conn_handle);
        if (attr->om != NULL) {
            uint16_t om_len = OS_MBUF_PKTLEN(attr->om);
            uint8_t data[om_len + 1];
            os_mbuf_copydata(attr->om, 0, om_len, data);
            data[om_len] = '\0';
            printf("Got data: %s\n", data);
        }
    } else {
        printf("Error reading characteristic; status=%d conn_handle=%d\n", 
                error->status, conn_handle);
    }
    return 0;
}

static int characteristic_write(uint16_t conn_handle, const struct ble_gatt_error *error,
                                    struct ble_gatt_attr *attr, void *arg)
{
    if (error->status == 0) {
        printf("Write characteristic completed; status=%d conn_handle=%d\n", 
                error->status, conn_handle);
    } else {
        printf("Error writing characteristic; status=%d conn_handle=%d\n", 
                error->status, conn_handle);
    }
    return 0;
}

static int characteristic_discovery(uint16_t conn_handle, const struct ble_gatt_error *error,
                                        const struct ble_gatt_chr *chr, void *arg)
{
    if (error->status == 0 && chr != NULL) {
        const ble_uuid_t *uuid = &chr->uuid.u;

        char uuid_str[BLE_UUID_STR_LEN];
        ble_uuid_to_str(uuid, uuid_str);
        printf("Characteristic discovered; UUID=%s\n", uuid_str);

        if (ble_uuid_cmp(uuid, BLE_UUID16_DECLARE(0xFEF4)) == 0) {
            printf("Found read characteristic with UUID 0xFEF4\n");
            ble_gattc_read(conn_handle, chr->val_handle, characteristic_read, NULL);
        } 
        else if (ble_uuid_cmp(uuid, BLE_UUID16_DECLARE(0xDEAD)) == 0) {
            printf("Found write characteristic with UUID 0xDEAD\n");
            const char *message = "Hello from client";
            ble_gattc_write_flat(conn_handle, chr->val_handle, message, 
                                 strlen(message), characteristic_write, NULL);
        }
    }
    return 0;
}

static int service_discovery(uint16_t conn_handle, const struct ble_gatt_error *error,
                                 const struct ble_gatt_svc *service, void *arg)
{
    if (error->status == 0 && service != NULL) {
        const ble_uuid_t *uuid = &service->uuid.u;

        char uuid_str[BLE_UUID_STR_LEN];
        ble_uuid_to_str(uuid, uuid_str);
        printf("Service discovered; UUID=%s\n", uuid_str);

        if (ble_uuid_cmp(uuid, BLE_UUID16_DECLARE(0x180)) == 0) {
            printf("Found our service with UUID 0x180\n");
            ble_gattc_disc_all_chrs(conn_handle, service->start_handle, 
                                    service->end_handle, characteristic_discovery, NULL);
        }
    }
    return 0;
}


static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_hs_adv_fields fields;
    
    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
            if (fields.name != NULL && 
                fields.name_len == strlen("BLE-Server") &&
                memcmp(fields.name, "BLE-Server", fields.name_len) == 0) {
                
                printf("Found server, connecting...\n");
                is_scanning = false;
                
                ble_gap_disc_cancel();
                ble_gap_connect(ble_addr_type, &event->disc.addr, 1000, NULL,
                                ble_gap_event, NULL);
            }
            break;

        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                printf("Connection established\n");
                is_connected_ble = true;
                
                ble_gattc_disc_all_svcs(event->connect.conn_handle, 
                                        service_discovery, NULL);
            } else {
                printf("Connection failed; status=%d\n", event->connect.status);

                ble_scan();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            printf("Disconnected; reason=%d\n", event->disconnect.reason);
            is_connected_ble = false;
            
            ble_scan();
            break;

        default:
            break;
    }
    return 0;
}

void ble_scan(void)
{
    if (!is_scanning) {
        struct ble_gap_disc_params disc_params;
        memset(&disc_params, 0, sizeof(disc_params));
        disc_params.filter_duplicates = 1;
        disc_params.passive = 0;
        disc_params.itvl = 0;
        disc_params.window = 0;
        disc_params.filter_policy = 0;
        disc_params.limited = 0;

        ble_gap_disc(ble_addr_type, BLE_HS_FOREVER, &disc_params, 
                     ble_gap_event, NULL);
        is_scanning = true;
    }
}

static void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_scan();
}

static void host_task(void *param)
{
    nimble_port_run();
}

void setup_ble_services(void) {
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set("BLE-Client");
}

void start_ble_services(void) {
    nimble_port_init();
    setup_ble_services();
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    nimble_port_freertos_init(host_task);
}











