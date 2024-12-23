#include "app_main.h"



void app_main(void)
{
    setup();
    
        if (wifi_is_connected())
        {
        http_get_request("example.com");
        printf("----------------------------------------\n");
        vTaskDelay(100);
        }

}
