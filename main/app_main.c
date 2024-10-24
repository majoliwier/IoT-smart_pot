#include "app_main.h"

void app_main(void)
{
    setup();
    
    while (true)
    {
        if (!wifi_is_connected())
        {
            reconnect_wifi();
        }

        if (wifi_is_connected())
        {
        http_get_request("http://example.com/");
        printf("----------------------------------------\n");
        vTaskDelay(100);
        }
    }
    
}
