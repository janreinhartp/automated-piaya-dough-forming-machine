#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "hmi_timers.h"
#include "machine_task.h"

#include "lvgl.h"
#include <stdint.h>
#include <stdio.h>

static lv_obj_t *s_info;

static void refresh(lv_timer_t *t)
{
    (void)t;
    machine_status_t st;
    machine_status_get(&st);
    char buf[192];
    uint32_t rem = (st.target_qty > st.current_qty) ? (st.target_qty - st.current_qty) : 0;
    snprintf(buf, sizeof(buf),
             "Target: %u\nCurrent: %u\nRemaining: %u\nStep: %s\nAuto active: %s",
             (unsigned)st.target_qty, (unsigned)st.current_qty, (unsigned)rem,
             st.step_name, st.auto_active ? "YES" : "no");
    lv_label_set_text(s_info, buf);
}

static void cmd_evt(lv_event_t *e)
{
    machine_cmd_send((machine_cmd_id_t)(uintptr_t)lv_event_get_user_data(e));
}

void hmi_show_auto(void)
{
    hmi_timers_stop();
    lv_obj_t *scr = hmi_screen_base();

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "AUTOMATIC PRODUCTION");
    lv_obj_set_style_text_color(title, HMI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 24, 16);

    s_info = lv_label_create(scr);
    lv_obj_set_style_text_color(s_info, HMI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_info, &lv_font_montserrat_20, 0);
    lv_obj_align(s_info, LV_ALIGN_TOP_LEFT, 24, 80);

    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_set_size(row, 980, 80);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 24, 280);
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

    lv_obj_t *nav = hmi_make_nav_bar(scr);
    hmi_attach_nav(nav);

    refresh(NULL);
    hmi_timers_set(lv_timer_create(refresh, 200, NULL));
    hmi_load_screen(scr);
}
