#include "hmi_init.h"
#include "hmi_theme.h"

#include "esp_log.h"

static const char *TAG = "hmi";

void hmi_init(void)
{
    hmi_theme_apply();
    ESP_LOGI(TAG, "showing Recovery gate");
    hmi_show_recovery();
}
