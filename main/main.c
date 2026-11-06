/*****************************************************************************
 * Piaya Dough Forming Machine — application entry
 * Preserves Waveshare board bring-up; replaces SquareLine demo UI with HMI.
 ******************************************************************************/
#include "rgb_lcd_port.h"
#include "gt911.h"
#include "lvgl_port.h"
#include "i2c.h"
#include "hmi_init.h"
#include "machine_task.h"
#include "relays.h"
#include "settings.h"
#include "history.h"
#include "safety.h"

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

static const char *TAG = "main";

static void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void app_main(void)
{
    init_nvs();

    machine_settings_t cfg;
    settings_load(&cfg);
    history_load(NULL);
    safety_init();

    esp_lcd_touch_handle_t tp_handle = touch_gt911_init();

    /* Bus is up after touch init — log every ACK before PCF8575 bring-up. */
    DEV_I2C_Scan();

    esp_lcd_panel_handle_t panel_handle = waveshare_esp32_s3_rgb_lcd_init();
    wavesahre_rgb_lcd_bl_on();
    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));

    esp_err_t io_err = relays_init(cfg.pcf8575_addr, cfg.relay_active_low);
    if (io_err != ESP_OK) {
        ESP_LOGE(TAG, "PCF8575/relay init failed (%s) — HMI will run; outputs unavailable",
                 esp_err_to_name(io_err));
    }

    ESP_ERROR_CHECK(machine_task_start());

    if (lvgl_port_lock(-1)) {
        hmi_init();
        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "Piaya HMI ready (recovery gate active)");
}
