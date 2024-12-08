#include "bh1750.h"

static const char *TAG_BH1750 = "bh1750";

#define I2C_ADDR 0x23
#define PIN_SDA 21
#define PIN_SCL 22

#define BH1750_MODE BH1750_CONTINUOUS_HIGH_RES_MODE


int bh1750_I2C_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);
int bh1750_I2C_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);

void bh1750_reset(void) {
    ESP_LOGE(TAG_BH1750, "reset");
    bh1750_I2C_write(I2C_ADDR, BH1750_POWER_ON, NULL, 0);
    bh1750_I2C_write(I2C_ADDR, BH1750_RESET, NULL, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

float bh1750_read(void) {
    uint8_t buf[2];
    uint8_t sleepms = 180;
    uint8_t resdiv = 1;
    float luxval = 0;
    int ret = -1;

    ret = bh1750_I2C_write(I2C_ADDR, BH1750_MODE, NULL, 0);
    if (ret != ESP_OK) {
        bh1750_reset();
        return -1;
    }

    vTaskDelay(sleepms / portTICK_PERIOD_MS);

    ret = bh1750_I2C_read(I2C_ADDR, 0xFF, buf, 2);
    if (ret != ESP_OK) {
        bh1750_reset();
        return -1;
    }

    uint16_t luxraw = (buf[0] << 8) | buf[1];
    luxval = (float)luxraw / 1.2 / resdiv;
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