#include "relays.h"

#include "esp_log.h"
#include "n4d8b08.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "relays";

static SemaphoreHandle_t s_mutex;
static uint8_t s_logical_on = 0; /* bit set => logical ON (last successfully commanded) */
static bool s_hw_warned;
/*
 * Set after any failed Modbus write: coil state is then unverified, so the
 * skip-if-unchanged optimisation is disabled until a close_all succeeds.
 */
static bool s_unverified;

static esp_err_t push_hw(relay_id_t id, bool logical_on)
{
    if (!n4d8b08_is_ready()) {
        if (!s_hw_warned) {
            ESP_LOGW(TAG, "N4D8B08 not ready");
            s_hw_warned = true;
        }
        s_unverified = true;
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t channel = (uint8_t)id + 1u;
    esp_err_t err = n4d8b08_set_channel(channel, logical_on);
    if (err != ESP_OK) {
        s_unverified = true;
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
    s_unverified = true;

    esp_err_t err = n4d8b08_init(slave_id, baud);
    if (err != ESP_OK) {
        return err;
    }
    return relays_force_safe();
}

esp_err_t relays_force_safe(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    /* Only claim "all off" if the close-all write succeeded. */
    esp_err_t err = n4d8b08_close_all();
    if (err == ESP_OK) {
        s_logical_on = 0;
        s_unverified = false;
    } else {
        s_unverified = true;
        ESP_LOGE(TAG, "close_all failed (%s); commanded mask left at 0x%02X (unverified)",
                 esp_err_to_name(err), (unsigned)s_logical_on);
    }
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t relays_set(relay_id_t id, bool logical_on)
{
    if (id >= RELAY_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    /* Press mutual exclusion: always close the opposite direction before opening one. */
    relay_id_t opposite = RELAY_COUNT;
    if (logical_on && id == RELAY_R3_PRESS_FWD) {
        opposite = RELAY_R4_PRESS_REV;
    }
    if (logical_on && id == RELAY_R4_PRESS_REV) {
        opposite = RELAY_R3_PRESS_FWD;
    }
    if (opposite < RELAY_COUNT) {
        /* Skip Modbus close only when opposite is known to be off. */
        if (s_unverified || (s_logical_on & (1u << opposite)) != 0) {
            esp_err_t err = push_hw(opposite, false);
            if (err != ESP_OK) {
                xSemaphoreGive(s_mutex);
                return err;
            }
            s_logical_on &= (uint8_t)~(1u << opposite);
        }
    }

    /* Skip redundant Modbus I/O when the verified commanded state already matches. */
    bool already_on = (s_logical_on & (1u << id)) != 0;
    if (!s_unverified && already_on == logical_on) {
        xSemaphoreGive(s_mutex);
        return ESP_OK;
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
    if (err != ESP_OK) {
        /* Hardware state unverified: leave commanded mask untouched. */
        s_unverified = true;
        ESP_LOGE(TAG, "apply_mask close_all failed: %s", esp_err_to_name(err));
        xSemaphoreGive(s_mutex);
        return err;
    }
    s_logical_on = 0;
    s_unverified = false;

    for (int i = 0; i < RELAY_COUNT; i++) {
        if (logical_on_mask & (1u << i)) {
            err = n4d8b08_set_channel((uint8_t)(i + 1), true);
            if (err != ESP_OK) {
                s_unverified = true;
                break;
            }
            s_logical_on |= (uint8_t)(1u << i);
        }
    }
    xSemaphoreGive(s_mutex);
    return err;
}
