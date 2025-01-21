#ifndef MQTT_H
#define MQTT_H
#define bad_led_pin 12


#include "mqtt_client.h"  // Nagłówek ESP-IDF do obsługi klienta MQTT

// Deklaracja globalnej zmiennej klienta MQTT
extern esp_mqtt_client_handle_t client;

// Deklaracja globalnej flagi stanu połączenia MQTT
extern bool mqtt_connected;

// Funkcje związane z obsługą MQTT
void mqtt_app_start(void);  // Funkcja do uruchamiania klienta MQTT
void mqtt_publish(const char *topic, const char *payload);  // Funkcja do publikowania wiadomości
void mqtt_message_handler(const char *topic, const char *payload);  // Funkcja obsługująca odbierane wiadomości

//Funckja do zastopowania i zniszczenia klienta mqtt
void mqtt_app_stop(void);

#endif  // MQTT_H
