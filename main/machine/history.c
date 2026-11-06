#include "history.h"

#include "esp_log.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "history";
static const char *NVS_NS = "piaya_hist";

static production_history_t s_hist;
static SemaphoreHandle_t s_mutex;
static bool s_loaded = false;

static void ensure_mutex(void)
{
    if (s_mutex == NULL) {
        s_mutex = xSemaphoreCreateMutex();
    }
}

esp_err_t history_load(production_history_t *out)
{
    ensure_mutex();
    s_hist = (production_history_t){0};

    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u32(h, "last_qty", &s_hist.last_qty);
        nvs_get_u32(h, "total_cycles", &s_hist.total_cycles);
        nvs_get_u32(h, "last_duration_ms", &s_hist.last_duration_ms);
        nvs_get_u32(h, "last_unix", &s_hist.last_unix);
        nvs_close(h);
    } else {
        ESP_LOGW(TAG, "no history yet");
    }

    s_loaded = true;
    if (out) {
        *out = s_hist;
    }
    return ESP_OK;
}

esp_err_t history_record_run(uint32_t qty, uint32_t duration_ms, uint32_t unix_ts)
{
    ensure_mutex();
    if (!s_loaded) {
        history_load(NULL);
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_hist.last_qty = qty;
    s_hist.last_duration_ms = duration_ms;
    s_hist.last_unix = unix_ts;
    s_hist.total_cycles += qty;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err == ESP_OK) {
        nvs_set_u32(h, "last_qty", s_hist.last_qty);
        nvs_set_u32(h, "total_cycles", s_hist.total_cycles);
        nvs_set_u32(h, "last_duration_ms", s_hist.last_duration_ms);
        nvs_set_u32(h, "last_unix", s_hist.last_unix);
        nvs_commit(h);
        nvs_close(h);
    }
    xSemaphoreGive(s_mutex);
    return err;
}

void history_get(production_history_t *out)
{
    ensure_mutex();
    if (!s_loaded) {
        history_load(NULL);
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    *out = s_hist;
    xSemaphoreGive(s_mutex);
}
