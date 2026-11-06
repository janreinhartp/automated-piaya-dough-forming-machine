#include "relays.h"

#include "esp_log.h"
#include "pcf8575.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "relays";

static SemaphoreHandle_t s_mutex;
static bool s_active_low = true;
static uint8_t s_logical_on = 0; /* bit set => logical ON */
static bool s_hw_warned;

static uint8_t logical_to_port0(uint8_t logical_on)
{
    if (s_active_low) {
        return (uint8_t)(~logical_on);
    }
    return logical_on;
}

static esp_err_t push_hw(void)
{
    if (!pcf8575_is_ready()) {
        if (!s_hw_warned) {
            ESP_LOGW(TAG, "PCF8575 not ready — keeping commanded state in RAM only");
            s_hw_warned = true;
        }
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t port0 = logical_to_port0(s_logical_on);
    ESP_LOGI(TAG, "push logical=0x%02X -> port0=0x%02X (active_low=%d)",
             s_logical_on, port0, (int)s_active_low);

    esp_err_t err = pcf8575_write_port0(port0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PCF8575 write failed (commanded mask kept in RAM: 0x%02X)", s_logical_on);
    } else {
        s_hw_warned = false;
    }
    return err;
}

esp_err_t relays_init(uint8_t pcf_addr, bool active_low)
{
    if (s_mutex == NULL) {
        s_mutex = xSemaphoreCreateMutex();
        if (s_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    s_active_low = active_low;
    s_logical_on = 0;

    esp_err_t err = pcf8575_init(pcf_addr);
    if (err != ESP_OK) {
        return err;
    }
    return relays_force_safe();
}

esp_err_t relays_set_polarity(bool active_low)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_active_low = active_low;
    esp_err_t err = push_hw();
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t relays_force_safe(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_logical_on = 0;
    esp_err_t err = push_hw();
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t relays_set(relay_id_t id, bool logical_on)
{
    if (id >= RELAY_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    /* Press mutual exclusion */
    if (logical_on && id == RELAY_R3_PRESS_FWD) {
        s_logical_on &= (uint8_t)~(1u << RELAY_R4_PRESS_REV);
    }
    if (logical_on && id == RELAY_R4_PRESS_REV) {
        s_logical_on &= (uint8_t)~(1u << RELAY_R3_PRESS_FWD);
    }

    if (logical_on) {
        s_logical_on |= (uint8_t)(1u << id);
    } else {
        s_logical_on &= (uint8_t)~(1u << id);
    }

    /* Never allow both press bits */
    if ((s_logical_on & (1u << RELAY_R3_PRESS_FWD)) &&
        (s_logical_on & (1u << RELAY_R4_PRESS_REV))) {
        ESP_LOGE(TAG, "press mutex violation blocked");
        s_logical_on &= (uint8_t)~((1u << RELAY_R3_PRESS_FWD) | (1u << RELAY_R4_PRESS_REV));
        push_hw();
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = push_hw();
    xSemaphoreGive(s_mutex);
    return err;
}

bool relays_get(relay_id_t id)
{
    if (id >= RELAY_COUNT) {
        return false;
    }
    return (s_logical_on & (1u << id)) != 0;
}

uint8_t relays_get_commanded_mask(void)
{
    return s_logical_on;
}

esp_err_t relays_apply_mask(uint8_t logical_on_mask)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if ((logical_on_mask & (1u << RELAY_R3_PRESS_FWD)) &&
        (logical_on_mask & (1u << RELAY_R4_PRESS_REV))) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_ARG;
    }
    s_logical_on = logical_on_mask;
    esp_err_t err = push_hw();
    xSemaphoreGive(s_mutex);
    return err;
}
