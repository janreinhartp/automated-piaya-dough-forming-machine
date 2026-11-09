#include "relays.h"

#include "esp_log.h"
#include "n4d8b08.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "relays";

static SemaphoreHandle_t s_mutex;
static uint8_t s_logical_on = 0; /* bit set => logical ON */
static bool s_hw_warned;

static esp_err_t push_hw(relay_id_t id, bool logical_on)
{
    if (!n4d8b08_is_ready()) {
        if (!s_hw_warned) {
            ESP_LOGW(TAG, "N4D8B08 not ready");
            s_hw_warned = true;
        }
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t channel = (uint8_t)id + 1u;
    esp_err_t err = n4d8b08_set_channel(channel, logical_on);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "N4D8B08 channel %u %s failed: %s",
                 (unsigned)channel, logical_on ? "open" : "close", esp_err_to_name(err));
    } else {
        s_hw_warned = false;
    }
    return err;
}

esp_err_t relays_init(uint8_t slave_id, uint32_t baud)
{
    if (s_mutex == NULL) {
        s_mutex = xSemaphoreCreateMutex();
        if (s_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    s_logical_on = 0;

    esp_err_t err = n4d8b08_init(slave_id, baud);
    if (err != ESP_OK) {
        return err;
    }
    return relays_force_safe();
}

esp_err_t relays_force_safe(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_logical_on = 0;
    esp_err_t err = n4d8b08_close_all();
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
    relay_id_t opposite = RELAY_COUNT;
    if (logical_on && id == RELAY_R3_PRESS_FWD) {
        opposite = RELAY_R4_PRESS_REV;
    }
    if (logical_on && id == RELAY_R4_PRESS_REV) {
        opposite = RELAY_R3_PRESS_FWD;
    }
    if (opposite < RELAY_COUNT) {
        esp_err_t err = push_hw(opposite, false);
        if (err != ESP_OK) {
            xSemaphoreGive(s_mutex);
            return err;
        }
        s_logical_on &= (uint8_t)~(1u << opposite);
    }

    esp_err_t err = push_hw(id, logical_on);
    if (err == ESP_OK) {
        if (logical_on) {
            s_logical_on |= (uint8_t)(1u << id);
        } else {
            s_logical_on &= (uint8_t)~(1u << id);
        }
    }
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

    esp_err_t err = n4d8b08_close_all();
    s_logical_on = 0;
    if (err == ESP_OK) {
        for (int i = 0; i < RELAY_COUNT; i++) {
            if (logical_on_mask & (1u << i)) {
                err = n4d8b08_set_channel((uint8_t)(i + 1), true);
                if (err != ESP_OK) {
                    break;
                }
                s_logical_on |= (uint8_t)(1u << i);
            }
        }
    }
    xSemaphoreGive(s_mutex);
    return err;
}
