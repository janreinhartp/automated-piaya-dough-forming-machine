#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "hmi_timers.h"
#include "machine_task.h"

#include "lvgl.h"
#include <stdint.h>
#include <stdio.h>

static lv_obj_t *s_status;
static lv_obj_t *s_step;
static lv_obj_t *s_qty;
static lv_obj_t *s_bar;

static void refresh_cb(lv_timer_t *t)
{
    (void)t;
    machine_status_t st;
    machine_status_get(&st);

    char buf[128];
    snprintf(buf, sizeof(buf), "Status: %s | Recovery: %s | Auto: %s",
             st.step_name,
             st.recovery_unlocked ? "Unlocked" : "LOCKED",
             st.auto_active ? "Running" : "No");
    lv_label_set_text(s_status, buf);

    snprintf(buf, sizeof(buf), "Step: %s  (remaining ~%u ms)",
             st.step_name, (unsigned)st.remaining_ms);
    lv_label_set_text(s_step, buf);

    snprintf(buf, sizeof(buf), "Production: %u / %u",
             (unsigned)st.current_qty, (unsigned)st.target_qty);
    lv_label_set_text(s_qty, buf);

    uint32_t pct = 0;
    if (st.target_qty > 0) {
        pct = (st.current_qty * 100) / st.target_qty;
        if (pct > 100) {
            pct = 100;
        }
    }
    lv_bar_set_value(s_bar, (int32_t)pct, LV_ANIM_OFF);
}

static void cmd_evt(lv_event_t *e)
{
    machine_cmd_id_t id = (machine_cmd_id_t)(uintptr_t)lv_event_get_user_data(e);
    machine_cmd_send(id);
}

void hmi_show_dashboard(void)
{
    hmi_timers_stop();
    lv_obj_t *scr = hmi_screen_base();

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "PIAYA DOUGH FORMING — DASHBOARD");
    lv_obj_set_style_text_color(title, HMI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 24, 16);

    s_status = lv_label_create(scr);
    lv_obj_set_style_text_color(s_status, HMI_COLOR_MUTED, 0);
    lv_obj_align(s_status, LV_ALIGN_TOP_LEFT, 24, 60);

    s_step = lv_label_create(scr);
    lv_obj_set_style_text_color(s_step, HMI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_step, &lv_font_montserrat_20, 0);
    lv_obj_align(s_step, LV_ALIGN_TOP_LEFT, 24, 110);

    s_qty = lv_label_create(scr);
    lv_obj_set_style_text_color(s_qty, HMI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_qty, &lv_font_montserrat_20, 0);
    lv_obj_align(s_qty, LV_ALIGN_TOP_LEFT, 24, 150);

    s_bar = lv_bar_create(scr);
    lv_obj_set_size(s_bar, 900, 28);
    lv_obj_align(s_bar, LV_ALIGN_TOP_LEFT, 24, 200);
    lv_bar_set_range(s_bar, 0, 100);

    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_set_size(row, 980, 80);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 24, 260);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 16, 0);

    lv_obj_t *b;
    b = hmi_make_btn(row, "START", HMI_COLOR_RUN);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_START_AUTO);
    b = hmi_make_btn(row, "PAUSE", HMI_COLOR_PAUSE);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_PAUSE);
    b = hmi_make_btn(row, "RESUME", HMI_COLOR_ACCENT);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_RESUME);
    b = hmi_make_btn(row, "STOP", HMI_COLOR_FAULT);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_STOP);

    lv_obj_t *note = lv_label_create(scr);
    lv_label_set_text(note, "Outputs shown elsewhere are COMMANDED states — not verified positions.");
    lv_obj_set_style_text_color(note, HMI_COLOR_MUTED, 0);
    lv_obj_align(note, LV_ALIGN_TOP_LEFT, 24, 360);

    lv_obj_t *nav = hmi_make_nav_bar(scr);
    hmi_attach_nav(nav);

    refresh_cb(NULL);
    hmi_timers_set(lv_timer_create(refresh_cb, 200, NULL));
    hmi_load_screen(scr);
}
