#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "hmi_timers.h"
#include "machine_task.h"

#include "lvgl.h"
#include <stdio.h>

static lv_obj_t *s_strip_step;
static lv_obj_t *s_strip_rec;
static lv_obj_t *s_strip_auto;
static lv_obj_t *s_step_name;
static lv_obj_t *s_step_note;
static lv_obj_t *s_remain;
static lv_obj_t *s_qty;
static lv_obj_t *s_tgt;
static lv_obj_t *s_pct;
static lv_obj_t *s_bar;
static lv_obj_t *s_ready;

static void refresh_cb(lv_timer_t *t)
{
    (void)t;
    machine_status_t st;
    machine_status_get(&st);

    char buf[96];
    snprintf(buf, sizeof(buf), "MACHINE STEP / %s", st.step_name ? st.step_name : "—");
    lv_label_set_text(s_strip_step, buf);

    lv_label_set_text(s_strip_rec, st.recovery_unlocked ? "RECOVERY / UNLOCKED" : "RECOVERY / LOCKED");
    lv_obj_set_style_text_color(s_strip_rec,
                                st.recovery_unlocked ? HMI_COLOR_OK : HMI_COLOR_WARN, 0);

    lv_label_set_text(s_strip_auto, st.auto_active ? "AUTO RUNNING / YES" : "AUTO RUNNING / NO");

    lv_label_set_text(s_step_name, st.step_name ? st.step_name : "IDLE");
    if (st.fault != FAULT_NONE) {
        lv_label_set_text(s_step_note, "Fault latched — clear before restart.");
        lv_label_set_text(s_ready, "FAULT");
        lv_obj_set_style_text_color(s_ready, HMI_COLOR_FAULT, 0);
    } else if (st.auto_active) {
        lv_label_set_text(s_step_note, "Automatic sequence in progress.");
        lv_label_set_text(s_ready, "RUNNING");
        lv_obj_set_style_text_color(s_ready, HMI_COLOR_ACCENT, 0);
    } else {
        lv_label_set_text(s_step_note, "Ready for a new production batch.");
        lv_label_set_text(s_ready, "READY");
        lv_obj_set_style_text_color(s_ready, HMI_COLOR_OK, 0);
    }

    snprintf(buf, sizeof(buf), "%u", (unsigned)st.remaining_ms);
    lv_label_set_text(s_remain, buf);

    snprintf(buf, sizeof(buf), "%u", (unsigned)st.current_qty);
    lv_label_set_text(s_qty, buf);
    snprintf(buf, sizeof(buf), "/ %u", (unsigned)st.target_qty);
    lv_label_set_text(s_tgt, buf);

    uint32_t pct = 0;
    if (st.target_qty > 0) {
        pct = (st.current_qty * 100) / st.target_qty;
        if (pct > 100) {
            pct = 100;
        }
    }
    snprintf(buf, sizeof(buf), "%u%%", (unsigned)pct);
    lv_label_set_text(s_pct, buf);
    lv_bar_set_value(s_bar, (int32_t)pct, LV_ANIM_OFF);
}

static void cmd_evt(lv_event_t *e)
{
    machine_cmd_send((machine_cmd_id_t)(uintptr_t)lv_event_get_user_data(e));
}

void hmi_show_dashboard(void)
{
    hmi_timers_stop();
    lv_obj_t *scr = hmi_screen_base();
    hmi_make_header(scr, "PIAYA DOUGH FORMING — DASHBOARD", false);

    lv_obj_t *strip = lv_obj_create(scr);
    lv_obj_set_size(strip, 992, 32);
    lv_obj_set_pos(strip, 16, 72);
    lv_obj_set_style_bg_color(strip, HMI_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(strip, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(strip, 5, 0);
    lv_obj_set_style_border_width(strip, 0, 0);
    lv_obj_set_style_pad_all(strip, 0, 0);
    lv_obj_clear_flag(strip, LV_OBJ_FLAG_SCROLLABLE);

    s_strip_step = hmi_make_label(strip, "MACHINE STEP / IDLE", HMI_COLOR_SOFT, &lv_font_montserrat_12);
    lv_obj_set_pos(s_strip_step, 12, 8);
    s_strip_rec = hmi_make_label(strip, "RECOVERY / LOCKED", HMI_COLOR_WARN, &lv_font_montserrat_12);
    lv_obj_set_pos(s_strip_rec, 357, 8);
    s_strip_auto = hmi_make_label(strip, "AUTO RUNNING / NO", HMI_COLOR_SOFT, &lv_font_montserrat_12);
    lv_obj_set_pos(s_strip_auto, 720, 8);

    lv_obj_t *step = hmi_make_card(scr, 16, 120, 600, 224);
    hmi_make_label(step, "CURRENT STEP", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(step, 0), 20, 20);
    s_step_name = hmi_make_label(step, "IDLE", HMI_COLOR_TEXT, &lv_font_montserrat_44);
    lv_obj_set_pos(s_step_name, 20, 46);
    s_step_note = hmi_make_label(step, "Ready for a new production batch.", HMI_COLOR_SOFT,
                                 &lv_font_montserrat_14);
    lv_obj_set_pos(s_step_note, 20, 100);
    lv_obj_t *div = lv_obj_create(step);
    lv_obj_set_size(div, 560, 1);
    lv_obj_set_pos(div, 20, 136);
    lv_obj_set_style_bg_color(div, HMI_COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);
    hmi_make_label(step, "STEP REMAINING", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(step, lv_obj_get_child_cnt(step) - 1), 20, 156);
    s_remain = hmi_make_label(step, "0", HMI_COLOR_ACCENT, &lv_font_montserrat_24);
    lv_obj_set_pos(s_remain, 20, 174);
    hmi_make_label(step, "ms", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(step, lv_obj_get_child_cnt(step) - 1), 90, 184);

    lv_obj_t *prod = hmi_make_card(scr, 632, 120, 376, 224);
    hmi_make_label(prod, "PRODUCTION / PIECES", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(prod, 0), 20, 20);
    s_qty = hmi_make_label(prod, "0", HMI_COLOR_TEXT, &lv_font_montserrat_44);
    lv_obj_set_pos(s_qty, 20, 46);
    s_tgt = hmi_make_label(prod, "/ 0", HMI_COLOR_MUTED, &lv_font_montserrat_20);
    lv_obj_set_pos(s_tgt, 90, 64);
    hmi_make_label(prod, "QUANTITY PROGRESS", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(prod, lv_obj_get_child_cnt(prod) - 1), 20, 120);
    s_pct = hmi_make_label(prod, "0%", HMI_COLOR_ACCENT, &lv_font_montserrat_14);
    lv_obj_align(s_pct, LV_ALIGN_TOP_RIGHT, -20, 118);
    s_bar = lv_bar_create(prod);
    lv_obj_set_size(s_bar, 336, 10);
    lv_obj_set_pos(s_bar, 20, 148);
    lv_bar_set_range(s_bar, 0, 100);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x0A1220), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar, HMI_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_bar, 5, 0);

    lv_obj_t *ready = lv_obj_create(scr);
    lv_obj_set_size(ready, 992, 44);
    lv_obj_set_pos(ready, 16, 360);
    lv_obj_set_style_bg_color(ready, HMI_COLOR_READY_BG, 0);
    lv_obj_set_style_bg_opa(ready, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ready, 5, 0);
    lv_obj_set_style_border_width(ready, 0, 0);
    lv_obj_set_style_pad_all(ready, 0, 0);
    lv_obj_clear_flag(ready, LV_OBJ_FLAG_SCROLLABLE);
    s_ready = hmi_make_label(ready, "READY", HMI_COLOR_OK, &lv_font_montserrat_12);
    lv_obj_set_pos(s_ready, 12, 14);
    hmi_make_label(ready, "Start Automatic, or open Manual Control to inspect individual commands.",
                   HMI_COLOR_SOFT, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(ready, 1), 85, 14);

    lv_obj_t *b;
    b = hmi_make_btn_sized(scr, "START", HMI_COLOR_RUN, 236, 64, HMI_COLOR_TEXT);
    lv_obj_set_pos(b, 16, 424);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_START_AUTO);
    b = hmi_make_btn_sized(scr, "PAUSE", HMI_COLOR_PAUSE_BG, 236, 64, HMI_COLOR_PAUSE_FG);
    lv_obj_set_pos(b, 268, 424);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_PAUSE);
    b = hmi_make_btn_sized(scr, "RESUME", HMI_COLOR_RESUME_BG, 236, 64, HMI_COLOR_RESUME_FG);
    lv_obj_set_pos(b, 520, 424);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_RESUME);
    b = hmi_make_btn_sized(scr, "STOP", HMI_COLOR_FAULT, 236, 64, HMI_COLOR_TEXT);
    lv_obj_set_pos(b, 772, 424);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_STOP);

    hmi_make_label(scr, "Commanded outputs only — not position feedback. Cylinders are timed.",
                   HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(scr, lv_obj_get_child_cnt(scr) - 1), 16, 502);

    hmi_make_nav_bar(scr, HMI_NAV_DASHBOARD);
    refresh_cb(NULL);
    hmi_timers_set(lv_timer_create(refresh_cb, 200, NULL));
    hmi_load_screen(scr);
}
