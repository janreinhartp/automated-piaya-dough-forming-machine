#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "hmi_timers.h"
#include "machine_drawing.h"
#include "machine_task.h"

#include "lvgl.h"
#include <stdio.h>

static lv_obj_t *s_target;
static lv_obj_t *s_current;
static lv_obj_t *s_remain_qty;
static lv_obj_t *s_auto_active;
static lv_obj_t *s_step;
static lv_obj_t *s_step_ms;
static lv_obj_t *s_drawing;

static void go_run_view(lv_event_t *e)
{
    (void)e;
    machine_cmd_send(CMD_START_AUTO);
    hmi_show_auto_run();
}

static void refresh(lv_timer_t *t)
{
    (void)t;
    machine_status_t st;
    machine_status_get(&st);

    char buf[48];
    snprintf(buf, sizeof(buf), "%u", (unsigned)st.target_qty);
    lv_label_set_text(s_target, buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)st.current_qty);
    lv_label_set_text(s_current, buf);
    uint32_t rem = (st.target_qty > st.current_qty) ? (st.target_qty - st.current_qty) : 0;
    snprintf(buf, sizeof(buf), "%u", (unsigned)rem);
    lv_label_set_text(s_remain_qty, buf);
    lv_label_set_text(s_auto_active, st.auto_active ? "YES" : "NO");
    lv_obj_set_style_text_color(s_auto_active,
                                st.auto_active ? HMI_COLOR_OK : HMI_COLOR_MUTED, 0);
    lv_label_set_text(s_step, st.step_name ? st.step_name : "IDLE");
    snprintf(buf, sizeof(buf), "%.1f s", st.remaining_ms / 1000.0f);
    lv_label_set_text(s_step_ms, buf);
    hmi_machine_drawing_update(s_drawing, &st);
}

static void cmd_evt(lv_event_t *e)
{
    machine_cmd_send((machine_cmd_id_t)(uintptr_t)lv_event_get_user_data(e));
}

static lv_obj_t *metric_card(lv_obj_t *parent, lv_coord_t x, const char *eyebrow)
{
    lv_obj_t *c = hmi_make_card(parent, x, 72, 236, 64);
    lv_obj_set_style_radius(c, 6, 0);
    lv_obj_t *e = hmi_make_label(c, eyebrow, HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(e, 13, 9);
    return c;
}

void hmi_show_auto(void)
{
    hmi_timers_stop();
    lv_obj_t *scr = hmi_screen_base();
    hmi_make_header(scr, "AUTOMATIC PRODUCTION", false);

    lv_obj_t *c;
    c = metric_card(scr, 16, "TARGET");
    s_target = hmi_make_label(c, "0", HMI_COLOR_TEXT, &lv_font_montserrat_24);
    lv_obj_set_pos(s_target, 13, 27);
    hmi_make_label(c, "pieces", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(c, 2), 70, 39);

    c = metric_card(scr, 268, "CURRENT");
    s_current = hmi_make_label(c, "0", HMI_COLOR_TEXT, &lv_font_montserrat_24);
    lv_obj_set_pos(s_current, 13, 27);
    hmi_make_label(c, "pieces", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(c, 2), 70, 39);

    c = metric_card(scr, 520, "REMAINING");
    s_remain_qty = hmi_make_label(c, "0", HMI_COLOR_TEXT, &lv_font_montserrat_24);
    lv_obj_set_pos(s_remain_qty, 13, 27);
    hmi_make_label(c, "pieces", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(c, 2), 70, 39);

    c = metric_card(scr, 772, "AUTO ACTIVE");
    s_auto_active = hmi_make_label(c, "NO", HMI_COLOR_MUTED, &lv_font_montserrat_24);
    lv_obj_set_pos(s_auto_active, 13, 27);
    hmi_make_label(c, "status", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(c, 2), 90, 39);

    s_drawing = hmi_machine_drawing_create(scr, 16, 152, 640, 252, false);

    lv_obj_t *live = hmi_make_card(scr, 672, 152, 336, 252);
    hmi_make_label(live, "STEP NAME", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(live, 0), 15, 15);
    s_step = hmi_make_label(live, "IDLE", HMI_COLOR_TEXT, &lv_font_montserrat_24);
    lv_obj_set_pos(s_step, 15, 39);
    hmi_make_label(live, "STEP REMAINING", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(live, 2), 15, 83);
    s_step_ms = hmi_make_label(live, "0.0 s", HMI_COLOR_ACCENT, &lv_font_montserrat_20);
    lv_obj_set_pos(s_step_ms, 15, 103);

    lv_obj_t *run = hmi_make_btn_sized(live, "Run Auto - drawing view", HMI_COLOR_MOTION,
                                       304, 56, HMI_COLOR_ACCENT);
    lv_obj_set_pos(run, 15, 144);
    lv_obj_add_event_cb(run, go_run_view, LV_EVENT_CLICKED, NULL);
    hmi_make_label(live, "Open the step-by-step run view.", HMI_COLOR_MUTED,
                   &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(live, lv_obj_get_child_cnt(live) - 1), 15, 216);

    lv_obj_t *b;
    b = hmi_make_btn_sized(scr, "START", HMI_COLOR_RUN, 236, 64, HMI_COLOR_TEXT);
    lv_obj_set_pos(b, 16, 424);
    lv_obj_add_event_cb(b, go_run_view, LV_EVENT_CLICKED, NULL);
    b = hmi_make_btn_sized(scr, "PAUSE", HMI_COLOR_PAUSE_BG, 236, 64, HMI_COLOR_PAUSE_FG);
    lv_obj_set_pos(b, 268, 424);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_PAUSE);
    b = hmi_make_btn_sized(scr, "RESUME", HMI_COLOR_RESUME_BG, 236, 64, HMI_COLOR_RESUME_FG);
    lv_obj_set_pos(b, 520, 424);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_RESUME);
    b = hmi_make_btn_sized(scr, "STOP", HMI_COLOR_FAULT, 236, 64, HMI_COLOR_TEXT);
    lv_obj_set_pos(b, 772, 424);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_STOP);

    hmi_make_label(scr, "Commanded outputs only - not position feedback. Cylinders are timed.",
                   HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(scr, lv_obj_get_child_cnt(scr) - 1), 16, 502);

    hmi_make_nav_bar(scr, HMI_NAV_AUTO);
    refresh(NULL);
    hmi_timers_set(lv_timer_create(refresh, 200, NULL));
    hmi_load_screen(scr);
}
