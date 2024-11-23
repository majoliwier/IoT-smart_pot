#include "app_main.h"


void app_main(void)
{
    setup();
    
    // while (true)
    // {
    //     if (!wifi_is_connected())
    //     {
    //         reconnect_wifi();
    //     }

    //     if (wifi_is_connected())
    //     {
    //     http_get_request("example.com");
    //     printf("----------------------------------------\n");
    //     vTaskDelay(100);
    //     }
    // }

     if (!wifi_is_connected())
        {
            reconnect_wifi();
        }

        if (wifi_is_connected())
        {
        http_get_request("example.com");
        printf("----------------------------------------\n");
        vTaskDelay(100);
        }

    mqtt_app_start();

    // mqtt_app_start();
    


    
}
