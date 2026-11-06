#include "pcf8575.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c.h"

static const char *TAG = "pcf8575";

static i2c_master_dev_handle_t s_dev;
static uint8_t s_addr = PCF8575_DEFAULT_ADDR;
static uint8_t s_port0 = 0xFF;
static uint8_t s_port1 = 0xFF;
static bool s_ready = false;

static esp_err_t transmit_once(const uint8_t *data, size_t len)
{
    return i2c_master_transmit(s_dev, data, len, 200);
}

static esp_err_t transmit_with_retry(const uint8_t *data, size_t len)
{
    esp_err_t err = ESP_FAIL;

    for (int attempt = 1; attempt <= 4; attempt++) {
        err = transmit_once(data, len);
        if (err == ESP_OK) {
            return ESP_OK;
        }

        ESP_LOGW(TAG, "write attempt %d failed: %s", attempt, esp_err_to_name(err));

        /* After a NACK/INVALID_STATE the IDF master bus can stick — reset and retry. */
        i2c_master_bus_handle_t bus = DEV_I2C_Get_Bus();
        if (bus != NULL) {
            (void)i2c_master_bus_reset(bus);
        }
        vTaskDelay(pdMS_TO_TICKS(5 * attempt));
    }

    return err;
}

uint8_t pcf8575_detect_addr(void)
{
    i2c_master_bus_handle_t bus = DEV_I2C_Get_Bus();
    if (bus == NULL) {
        return 0;
    }

    /* PCF8575 A0/A1/A2 → 0x20..0x27. Skip 0x24 (Waveshare IO_EXTENSION). */
    for (uint8_t addr = 0x20; addr <= 0x27; addr++) {
        if (addr == 0x24) {
            continue;
        }
        if (i2c_master_probe(bus, addr, 50) == ESP_OK) {
            ESP_LOGI(TAG, "auto-detect: candidate at 0x%02X", addr);
            return addr;
        }
    }
    ESP_LOGW(TAG, "auto-detect: no PCF8575 candidate in 0x20..0x27 (except 0x24)");
    return 0;
}

esp_err_t pcf8575_init(uint8_t addr)
{
    s_ready = false;
    s_dev = NULL;

    i2c_master_bus_handle_t bus = DEV_I2C_Get_Bus();
    if (bus == NULL) {
        ESP_LOGE(TAG, "I2C bus not initialized (call touch/I2C bring-up first)");
        return ESP_ERR_INVALID_STATE;
    }

    if (addr == PCF8575_ADDR_AUTO) {
        s_addr = pcf8575_detect_addr();
        if (s_addr == 0) {
            ESP_LOGE(TAG, "auto-detect failed — set A0-A2 jumpers and check wiring");
            return ESP_ERR_NOT_FOUND;
        }
        ESP_LOGI(TAG, "using auto-detected address 0x%02X", s_addr);
    } else {
        s_addr = addr;
    }

    esp_err_t probe = i2c_master_probe(bus, s_addr, 100);
    if (probe != ESP_OK) {
        ESP_LOGE(TAG,
                 "No ACK at 0x%02X (%s). Check PCF8575 power, SDA/SCL to GPIO8/9, and A0-A2.",
                 s_addr, esp_err_to_name(probe));
        return probe;
    }

    /* 100 kHz is more tolerant next to GT911 traffic on the same bus. */
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = s_addr,
        .scl_speed_hz = 100000,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &cfg, &s_dev);
    if (err != ESP_OK || s_dev == NULL) {
        ESP_LOGE(TAG, "add_device 0x%02X failed: %s", s_addr, esp_err_to_name(err));
        s_dev = NULL;
        return (err != ESP_OK) ? err : ESP_FAIL;
    }

    /* Allow touch/LVGL to finish early I2C traffic before first write. */
    vTaskDelay(pdMS_TO_TICKS(20));

    /* Relays use Port0 only — single-byte writes avoid 2nd-byte NACKs on some modules. */
    err = pcf8575_write_port0(0xFF);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init write failed at 0x%02X: %s", s_addr, esp_err_to_name(err));
        return err;
    }

    s_ready = true;
    ESP_LOGI(TAG, "ready at 0x%02X", s_addr);
    return ESP_OK;
}

esp_err_t pcf8575_write_ports(uint8_t port0, uint8_t port1)
{
    if (s_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[2] = {port0, port1};
    ESP_LOGI(TAG, "write P0=0x%02X P1=0x%02X", port0, port1);

    esp_err_t err = transmit_with_retry(data, sizeof(data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "write failed after retries: %s", esp_err_to_name(err));
        return err;
    }

    s_port0 = port0;
    s_port1 = port1;
    s_ready = true;
    return ESP_OK;
}

esp_err_t pcf8575_write_port0(uint8_t port0)
{
    if (s_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "write P0=0x%02X", port0);

    esp_err_t err = transmit_with_retry(&port0, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "write P0 failed after retries: %s", esp_err_to_name(err));
        return err;
    }

    s_port0 = port0;
    s_ready = true;
    return ESP_OK;
}

esp_err_t pcf8575_read_ports(uint8_t *out_port0, uint8_t *out_port1)
{
    if (s_dev == NULL || out_port0 == NULL || out_port1 == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2] = {0};
    esp_err_t err = i2c_master_receive(s_dev, data, sizeof(data), 200);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(err));
        return err;
    }

    *out_port0 = data[0];
    *out_port1 = data[1];
    return ESP_OK;
}

bool pcf8575_is_ready(void)
{
    return s_ready && (s_dev != NULL);
}

uint8_t pcf8575_get_addr(void)
{
    return s_addr;
}
