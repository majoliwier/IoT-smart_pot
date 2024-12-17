#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Wybór numeru magistrali I2C
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

// Częstotliwość magistrali I2C
#define I2C_MASTER_FREQ_HZ 100000 

// Włącza sprawdzanie sygnału ACK podczas transmisji I2C.
#define ACK_CHECK_EN 0x1
#define ACK_VAL 0x0
#define NACK_VAL 0x1


// Czujnik przechodzi w stan nieaktywny.
#define BH1750_POWER_DOWN 0x00
// Czujnik włącza się i czeka na komendę pomiarową.
#define BH1750_POWER_ON 0x01

// Resetuje wewnętrzny rejestr danych czujnika. Nie działa w trybie Power Down.
#define BH1750_RESET 0x07

//Ustawienie górnych bitów wartości czasu pomiaru.
#define BH1750_MT_HI      0x40

//Ustawienie dolnych bitów wartości czasu pomiaru.
#define BH1750_MT_LO      0x60

//Tryb pomiaru ciągłego z wysoką rozdzielczością (1 lx, czas pomiaru ~120 ms).
#define BH1750_CONTINUOUS_HIGH_RES_MODE 0x10

//Tryb  pomiaru ciągłego z bardzo wysoką rozdzielczością (0.5 lx, czas pomiaru ~120 ms).
#define BH1750_CONTINUOUS_HIGH_RES_MODE2 0x11

//Tryb pomiaru ciągłego z niską rozdzielczością (4 lx, czas pomiaru ~16 ms).
#define BH1750_CONTINUOUS_LOW_RES_MODE 0x13

//Tryb jednorazowego pomiaru z wysoką rozdzielczością (1 lx, czas pomiaru ~120 ms).
#define BH1750_ONE_TIME_HIGH_RES_MODE     0x20

//Tryb jednorazowego pomiaru z bardzo wysoką rozdzielczością (0.5 lx, czas pomiaru ~120 ms).
#define BH1750_ONE_TIME_HIGH_RES_MODE2    0x21

//Tryb jednorazowego pomiaru z niską rozdzielczością (4 lx, czas pomiaru ~16 ms).
#define BH1750_ONE_TIME_LOW_RES_MODE      0x23

/** 
* @brief Inicjalizuje magistralę I2C i przygotowuje czujnik do pracy.
*/
void bh1750_init(void);


/**
 * @brief Odczytuje dane z czujnika w wybranym trybie.
 * 
 * @param mode Tryb pracy czujnika (np. BH1750_CONTINUOUS_HIGH_RES_MODE).
 * @return float Wynik pomiaru w luksach, lub -1 w przypadku błędu.
 */
float bh1750_read(uint8_t mode);


/**
 * @brief Funkcja zapisująca dane do urządzenia przez magistralę I2C.
 * 
 * @param dev_addr Adres I2C urządzenia.
 * @param reg_addr Adres rejestru w urządzeniu.
 * @param reg_data Wskaźnik na dane do zapisania.
 * @param cnt Liczba bajtów do zapisania.
 * @return int Wynik operacji (ESP_OK w przypadku sukcesu, lub kod błędu).
 */
int bh1750_I2C_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);


/**
 * @brief Funkcja odczytująca dane z urządzenia przez magistralę I2C.
 * 
 * @param dev_addr Adres I2C urządzenia.
 * @param reg_addr Adres rejestru w urządzeniu.
 * @param reg_data Wskaźnik na bufor do przechowywania odczytanych danych.
 * @param cnt Liczba bajtów do odczytu.
 * @return int Wynik operacji (ESP_OK w przypadku sukcesu, lub kod błędu).
 */
int bh1750_I2C_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);


/**
 * @brief Resetuje czujnik BH1750.
 */
void bh1750_reset(void);

/**
 * @brief Ustawia czas pomiaru czujnika.
 * 
 * @param mt Czas pomiaru (zakres 31-254).
 * @return esp_err_t ESP_OK w przypadku sukcesu, ESP_ERR_INVALID_ARG w przypadku błędnego zakresu.
 */
esp_err_t bh1750_set_measurement_time(uint8_t mt);


/**
 * @brief Odczyt w trybie ciągłym z wysoką rozdzielczością. (1lx)
 * @return float Wynik pomiaru w luksach.
 */
float bh1750_read_continuous_high_res(void);

/**
 * @brief Odczyt w trybie ciągłym z bardzo wysoką rozdzielczością. (0.5lx)
 * @return float Wynik pomiaru w luksach.
 */
float bh1750_read_continuous_high_res2(void);

/**
 * @brief Odczyt w trybie ciągłym z niską rozdzielczością. (4lx)
 * @return float Wynik pomiaru w luksach.
 */
float bh1750_read_continuous_low_res(void);


/**
 * @brief Odczyt jednorazowy z wysoką rozdzielczością. (1lx)
 * @return float Wynik pomiaru w luksach.
 */
float bh1750_read_one_time_high_res(void);

/**
 * @brief Odczyt jednorazowy z bardzo wysoką rozdzielczością. (0.5 lx)
 * @return float Wynik pomiaru w luksach.
 */
float bh1750_read_one_time_high_res2(void);

/**
 * @brief Odczyt jednorazowy z niską rozdzielczością. (4lx)
 * @return float Wynik pomiaru w luksach.
 */
float bh1750_read_one_time_low_res(void);

