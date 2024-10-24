#include "setup.h"

void setup(){
    nvs_flash_init();
    wifi_connection();
}