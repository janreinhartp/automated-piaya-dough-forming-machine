#include "hmi_timers.h"

#include "lvgl.h"

static lv_timer_t *s_ui_timer;

void hmi_timers_stop(void)
{
    if (s_ui_timer != NULL) {
        lv_timer_del(s_ui_timer);
        s_ui_timer = NULL;
    }
}

void hmi_timers_set(lv_timer_t *timer)
{
    hmi_timers_stop();
    s_ui_timer = timer;
}
