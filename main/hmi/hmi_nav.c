#include "hmi_init.h"
#include "hmi_theme.h"
#include "hmi_timers.h"
#include "machine_task.h"
#include "safety.h"

#include "lvgl.h"

static void nav_dash(lv_event_t *e) { (void)e; hmi_timers_stop(); hmi_show_dashboard(); }
static void nav_manual(lv_event_t *e)
{
    (void)e;
    if (!safety_is_recovery_unlocked()) {
        return;
    }
    hmi_timers_stop();
    hmi_show_manual();
}
static void nav_auto(lv_event_t *e)
{
    (void)e;
    if (!safety_is_recovery_unlocked()) {
        return;
    }
    hmi_timers_stop();
    hmi_show_auto();
}
static void nav_settings(lv_event_t *e) { (void)e; hmi_timers_stop(); hmi_show_settings(); }
static void nav_history(lv_event_t *e) { (void)e; hmi_timers_stop(); hmi_show_history(); }

void hmi_attach_nav(lv_obj_t *bar)
{
    lv_obj_t *b;
    b = hmi_make_btn(bar, "Dashboard", HMI_COLOR_ACCENT);
    lv_obj_set_size(b, 180, 48);
    lv_obj_add_event_cb(b, nav_dash, LV_EVENT_CLICKED, NULL);

    b = hmi_make_btn(bar, "Manual", HMI_COLOR_STOP);
    lv_obj_set_size(b, 140, 48);
    lv_obj_add_event_cb(b, nav_manual, LV_EVENT_CLICKED, NULL);

    b = hmi_make_btn(bar, "Auto", HMI_COLOR_RUN);
    lv_obj_set_size(b, 140, 48);
    lv_obj_add_event_cb(b, nav_auto, LV_EVENT_CLICKED, NULL);

    b = hmi_make_btn(bar, "Settings", HMI_COLOR_STOP);
    lv_obj_set_size(b, 140, 48);
    lv_obj_add_event_cb(b, nav_settings, LV_EVENT_CLICKED, NULL);

    b = hmi_make_btn(bar, "History", HMI_COLOR_STOP);
    lv_obj_set_size(b, 140, 48);
    lv_obj_add_event_cb(b, nav_history, LV_EVENT_CLICKED, NULL);
}

lv_obj_t *hmi_screen_base(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, HMI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    return scr;
}

void hmi_load_screen(lv_obj_t *scr)
{
    /* auto_del=true defers deletion of the previous screen safely after the event. */
    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
}
