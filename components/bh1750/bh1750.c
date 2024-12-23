#include "bh1750.h"

static const char *TAG_BH1750 = "bh1750";

//Adres I2C dla czujnika BH1750
#define I2C_ADDR 0x23
// Numer pinu GPIO dla linii danych SDA
#define PIN_SDA 21
// Numer pinu GPIO dla linii zegara SCL
#define PIN_SCL 22


int bh1750_I2C_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);
int bh1750_I2C_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);

uint8_t measurement_time = 69;

void bh1750_reset(void) {
    ESP_LOGE(TAG_BH1750, "reset");
    bh1750_I2C_write(I2C_ADDR, BH1750_POWER_ON, NULL, 0);
    bh1750_I2C_write(I2C_ADDR, BH1750_RESET, NULL, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

float bh1750_read(uint8_t mode) {
    uint8_t buf[2];
    uint8_t resdiv = 1;
    float luxval = 0;
    int ret = -1;

    ret = bh1750_I2C_write(I2C_ADDR, mode, NULL, 0);
    if (ret != ESP_OK) {
        bh1750_reset();
        return -1;
    }

    uint16_t sleepms;
    if (mode == BH1750_CONTINUOUS_LOW_RES_MODE || mode == BH1750_ONE_TIME_LOW_RES_MODE) {
        sleepms = (uint16_t)((float)measurement_time * 0.3);
    } else {
        sleepms = (uint16_t)((float)measurement_time * 2.4);
    }
    vTaskDelay(sleepms / portTICK_PERIOD_MS);


    ret = bh1750_I2C_read(I2C_ADDR, 0xFF, buf, 2);
    if (ret != ESP_OK) {
        bh1750_reset();
        return -1;
    }

    float mt_correction = (float)measurement_time / 69.0;
    uint16_t luxraw = (buf[0] << 8) | buf[1];

    
    luxval = ((float)luxraw / 1.2) / mt_correction;

    if (mode == BH1750_CONTINUOUS_HIGH_RES_MODE2 || mode == BH1750_ONE_TIME_HIGH_RES_MODE2) {
        luxval /= 2;
    }


    ESP_LOGI(TAG_BH1750, "lux_raw=%u lux=%f", luxraw, luxval);

    return luxval;
}

void bh1750_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = PIN_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };

    ESP_LOGI(TAG_BH1750, "Initializing I2C...");
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));
}

int bh1750_I2C_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt) {
    int ret = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG_BH1750, "Error in I2C write");
    }
    return ret;
}

int bh1750_I2C_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt) {
    int ret = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read(cmd, reg_data, cnt - 1, ACK_VAL);
    i2c_master_read_byte(cmd, reg_data + cnt - 1, NACK_VAL);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG_BH1750, "Error in I2C read");
    }
    return ret;
}

esp_err_t bh1750_set_measurement_time(uint8_t mt) {
    if (mt < 31 || mt > 254) {
        ESP_LOGE(TAG_BH1750, "Invalid measurement time: %d (valid range: 31-254)", mt);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    ret = bh1750_I2C_write(I2C_ADDR, BH1750_MT_HI | (mt >> 5), NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_BH1750, "Failed to set MT high bits");
        return ret;
    }

    ret = bh1750_I2C_write(I2C_ADDR, BH1750_MT_LO | (mt & 0x1F), NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_BH1750, "Failed to set MT low bits");
        return ret;
    }

    measurement_time = mt;
    ESP_LOGI(TAG_BH1750, "Measurement time set to: %d", measurement_time);
    return ESP_OK;
}

float bh1750_read_continuous_high_res(void) {
    return bh1750_read(BH1750_CONTINUOUS_HIGH_RES_MODE);
}

float bh1750_read_continuous_high_res2(void) {
    return bh1750_read(BH1750_CONTINUOUS_HIGH_RES_MODE2);
}

float bh1750_read_continuous_low_res(void) {
    return bh1750_read(BH1750_CONTINUOUS_LOW_RES_MODE);
}

float bh1750_read_one_time_high_res(void) {
    return bh1750_read(BH1750_ONE_TIME_HIGH_RES_MODE);
}

float bh1750_read_one_time_high_res2(void) {
    return bh1750_read(BH1750_ONE_TIME_HIGH_RES_MODE2);
}

float bh1750_read_one_time_low_res(void) {
    return bh1750_read(BH1750_ONE_TIME_LOW_RES_MODE);
}