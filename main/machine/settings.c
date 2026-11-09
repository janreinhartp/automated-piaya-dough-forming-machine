#include "settings.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "settings";
static const char *NVS_NS = "piaya_cfg";
static const uint32_t MAGIC = 0x50494159; /* PIAY */
static const uint16_t VERSION = 2;

static machine_settings_t s_cfg;
static SemaphoreHandle_t s_mutex;
static bool s_loaded = false;

void settings_get_defaults(machine_settings_t *out)
{
    out->mixing_duration_ms = 60000;
    out->door_open_duration_ms = 3000;
    out->door_close_duration_ms = 3000;
    out->dough_transfer_duration_ms = 2000;
    out->press_advancement_duration_ms = 500;
    out->press_hold_duration_ms = 500;
    out->cutter_extend_duration_ms = 800;
    out->cutter_return_duration_ms = 800;
    out->conveyor_duration_ms = 2000;
    out->flattener_extend_duration_ms = 1000;
    out->flattener_return_duration_ms = 1000;
    out->target_quantity = 50;
    out->modbus_slave_id = 1;
    out->modbus_baud = 9600;
}

bool settings_validate(const machine_settings_t *in)
{
    if (in == NULL) {
        return false;
    }
    if (in->mixing_duration_ms < 1000 || in->mixing_duration_ms > 3600000) return false;
    if (in->door_open_duration_ms < 200 || in->door_open_duration_ms > 60000) return false;
    if (in->door_close_duration_ms < 200 || in->door_close_duration_ms > 60000) return false;
    if (in->dough_transfer_duration_ms > 60000) return false;
    if (in->press_advancement_duration_ms < 50 || in->press_advancement_duration_ms > 10000) return false;
    if (in->press_hold_duration_ms > 30000) return false;
    if (in->cutter_extend_duration_ms < 50 || in->cutter_extend_duration_ms > 30000) return false;
    if (in->cutter_return_duration_ms < 50 || in->cutter_return_duration_ms > 30000) return false;
    if (in->conveyor_duration_ms < 100 || in->conveyor_duration_ms > 120000) return false;
    if (in->flattener_extend_duration_ms < 50 || in->flattener_extend_duration_ms > 30000) return false;
    if (in->flattener_return_duration_ms < 50 || in->flattener_return_duration_ms > 30000) return false;
    if (in->target_quantity < 1 || in->target_quantity > 100000) return false;
    if (in->modbus_slave_id < 1 || in->modbus_slave_id > 47) return false;
    if (in->modbus_baud != 2400 && in->modbus_baud != 4800 &&
        in->modbus_baud != 9600 && in->modbus_baud != 19200) return false;
    return true;
}

static void ensure_mutex(void)
{
    if (s_mutex == NULL) {
        s_mutex = xSemaphoreCreateMutex();
    }
}

esp_err_t settings_load(machine_settings_t *out)
{
    ensure_mutex();
    settings_get_defaults(&s_cfg);

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open failed, using defaults");
        if (out) {
            *out = s_cfg;
        }
        s_loaded = true;
        return ESP_OK;
    }

    uint32_t magic = 0;
    uint16_t ver = 0;
    size_t len = sizeof(s_cfg);
    machine_settings_t tmp;

    if (nvs_get_u32(h, "magic", &magic) != ESP_OK || magic != MAGIC ||
        nvs_get_u16(h, "version", &ver) != ESP_OK || ver != VERSION ||
        nvs_get_blob(h, "cfg", &tmp, &len) != ESP_OK || len != sizeof(tmp) ||
        !settings_validate(&tmp)) {
        ESP_LOGW(TAG, "invalid NVS settings, using defaults");
    } else {
        s_cfg = tmp;
    }

    nvs_close(h);
    s_loaded = true;
    if (out) {
        *out = s_cfg;
    }
    return ESP_OK;
}

esp_err_t settings_save(const machine_settings_t *in)
{
    if (!settings_validate(in)) {
        return ESP_ERR_INVALID_ARG;
    }

    ensure_mutex();
    xSemaphoreTake(s_mutex, portMAX_DELAY);

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        xSemaphoreGive(s_mutex);
        return err;
    }

    err = nvs_set_u32(h, "magic", MAGIC);
    if (err == ESP_OK) {
        err = nvs_set_u16(h, "version", VERSION);
    }
    if (err == ESP_OK) {
        err = nvs_set_blob(h, "cfg", in, sizeof(*in));
    }
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);

    if (err == ESP_OK) {
        s_cfg = *in;
        s_loaded = true;
    }

    xSemaphoreGive(s_mutex);
    return err;
}

void settings_get(machine_settings_t *out)
{
    ensure_mutex();
    if (!s_loaded) {
        settings_load(NULL);
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    *out = s_cfg;
    xSemaphoreGive(s_mutex);
}

esp_err_t settings_set(const machine_settings_t *in)
{
    if (!settings_validate(in)) {
        return ESP_ERR_INVALID_ARG;
    }
    ensure_mutex();
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_cfg = *in;
    s_loaded = true;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}
