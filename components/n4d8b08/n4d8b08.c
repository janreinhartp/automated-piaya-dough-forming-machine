#include "n4d8b08.h"
#include "modbus_crc.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "n4d8b08";
static uint8_t s_slave = N4D8B08_DEFAULT_SLAVE;
static bool s_ready;
static SemaphoreHandle_t s_mutex;

static esp_err_t fc06(uint16_t reg, uint16_t value)
{
    uint8_t req[8];
    req[0] = s_slave;
    req[1] = 0x06;
    req[2] = (uint8_t)(reg >> 8);
    req[3] = (uint8_t)(reg & 0xFF);
    req[4] = (uint8_t)(value >> 8);
    req[5] = (uint8_t)(value & 0xFF);
    uint16_t crc = modbus_crc16(req, 6);
    req[6] = (uint8_t)(crc & 0xFF);
    req[7] = (uint8_t)(crc >> 8);

    /* 3.5 char quiet time @9600 ≈ 4ms */
    vTaskDelay(pdMS_TO_TICKS(5));
    uart_flush_input(N4D8B08_UART_NUM);
    int w = uart_write_bytes(N4D8B08_UART_NUM, (const char *)req, 8);
    if (w != 8) return ESP_FAIL;
    (void)uart_wait_tx_done(N4D8B08_UART_NUM, pdMS_TO_TICKS(100));

    uint8_t resp[8];
    int r = uart_read_bytes(N4D8B08_UART_NUM, resp, 8, pdMS_TO_TICKS(200));
    if (r != 8) {
        ESP_LOGW(TAG, "FC06 timeout/short read %d (reg=0x%04X val=0x%04X)", r, reg, value);
        return ESP_ERR_TIMEOUT;
    }
    uint16_t rcrc = modbus_crc16(resp, 6);
    if (resp[6] != (rcrc & 0xFF) || resp[7] != (rcrc >> 8)) {
        return ESP_ERR_INVALID_CRC;
    }
    /* Expect echo of request for FC06 */
    for (int i = 0; i < 6; i++) {
        if (resp[i] != req[i]) return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}

static esp_err_t fc06_retry(uint16_t reg, uint16_t value)
{
    esp_err_t err = ESP_FAIL;
    for (int i = 0; i < 3; i++) {
        err = fc06(reg, value);
        if (err == ESP_OK) return ESP_OK;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return err;
}

esp_err_t n4d8b08_init(uint8_t slave_id, uint32_t baud)
{
    s_slave = slave_id ? slave_id : N4D8B08_DEFAULT_SLAVE;
    if (baud == 0) baud = N4D8B08_DEFAULT_BAUD;
    if (s_mutex == NULL) s_mutex = xSemaphoreCreateMutex();

    uart_config_t cfg = {
        .baud_rate = (int)baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(N4D8B08_UART_NUM, 256, 256, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(N4D8B08_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(N4D8B08_UART_NUM, N4D8B08_TX_GPIO, N4D8B08_RX_GPIO,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    s_ready = true;
    esp_err_t err = n4d8b08_close_all();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "close_all at init failed: %s (module may be unpowered)", esp_err_to_name(err));
        /* Still mark ready so HMI can retry; machine treats failures per command */
    }
    ESP_LOGI(TAG, "ready slave=%u baud=%lu TX=%d RX=%d", s_slave, (unsigned long)baud,
             N4D8B08_TX_GPIO, N4D8B08_RX_GPIO);
    return ESP_OK;
}

esp_err_t n4d8b08_set_channel(uint8_t channel_1_to_8, bool open)
{
    if (!s_ready || channel_1_to_8 < 1 || channel_1_to_8 > 8) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    esp_err_t err = fc06_retry(channel_1_to_8, open ? 0x0001 : 0x0002);
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t n4d8b08_close_all(void)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    esp_err_t err = fc06_retry(0x0000, 0x0008);
    xSemaphoreGive(s_mutex);
    return err;
}

bool n4d8b08_is_ready(void) { return s_ready; }
