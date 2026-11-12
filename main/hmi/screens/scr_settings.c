#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "hmi_timers.h"
#include "machine_task.h"
#include "settings.h"

#include "esp_err.h"
#include "lvgl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static machine_settings_t s_edit;
static lv_obj_t *s_msg;
static lv_obj_t *s_kb;
static lv_obj_t *s_list;

typedef struct {
    uint32_t *field;
    lv_obj_t *ta;
    bool as_seconds;
} field_bind_t;

static field_bind_t s_binds[12];
static int s_bind_count;

static void format_field(char *buf, size_t n, uint32_t value, bool as_seconds)
{
    if (!as_seconds) {
        snprintf(buf, n, "%u", (unsigned)value);
        return;
    }
    if (value % 1000u == 0u) {
        snprintf(buf, n, "%u", (unsigned)(value / 1000u));
    } else {
        snprintf(buf, n, "%.2f", value / 1000.0);
    }
}

static void ta_focus_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    if (s_kb == NULL) {
        return;
    }
    lv_keyboard_set_textarea(s_kb, ta);
    lv_obj_clear_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_kb);
}

static void kb_ready_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_keyboard_set_textarea(s_kb, NULL);
        lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static void add_field(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, const char *name,
                      uint32_t *field, bool as_seconds)
{
    lv_obj_t *lab = hmi_make_label(parent, name, HMI_COLOR_SOFT, &lv_font_montserrat_12);
    lv_obj_set_pos(lab, x, y);

    lv_obj_t *ta = lv_textarea_create(parent);
    lv_obj_set_size(ta, 452, 40);
    lv_obj_set_pos(ta, x, y + 22);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_accepted_chars(ta, as_seconds ? "0123456789." : "0123456789");
    lv_textarea_set_max_length(ta, 8);
    lv_obj_set_style_bg_color(ta, HMI_COLOR_FIELD, 0);
    lv_obj_set_style_border_color(ta, HMI_COLOR_FIELD_BD, 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta, 5, 0);
    lv_obj_set_style_text_color(ta, HMI_COLOR_TEXT, 0);
    char buf[16];
    format_field(buf, sizeof(buf), *field, as_seconds);
    lv_textarea_set_text(ta, buf);
    lv_obj_add_event_cb(ta, ta_focus_cb, LV_EVENT_FOCUSED, NULL);
    if (s_bind_count < 12) {
        s_binds[s_bind_count].field = field;
        s_binds[s_bind_count].ta = ta;
        s_binds[s_bind_count].as_seconds = as_seconds;
        s_bind_count++;
    }
}

static void add_pair(lv_obj_t *parent, lv_coord_t y, const char *lname, uint32_t *lfield,
                     bool l_secs, const char *rname, uint32_t *rfield, bool r_secs)
{
    add_field(parent, 16, y, lname, lfield, l_secs);
    add_field(parent, 486, y, rname, rfield, r_secs);
}

static void pull_fields(void)
{
    for (int i = 0; i < s_bind_count; i++) {
        const char *txt = lv_textarea_get_text(s_binds[i].ta);
        if (s_binds[i].as_seconds) {
            double sec = strtod(txt, NULL);
            if (sec < 0.0) {
                sec = 0.0;
            }
            *s_binds[i].field = (uint32_t)lround(sec * 1000.0);
        } else {
            *s_binds[i].field = (uint32_t)strtoul(txt, NULL, 10);
        }
    }
}

static void on_save(lv_event_t *e)
{
    (void)e;
    machine_status_t st;
    machine_status_get(&st);
    if (st.auto_active) {
        lv_label_set_text(s_msg, "Cannot save while auto running");
        lv_obj_set_style_text_color(s_msg, HMI_COLOR_WARN, 0);
        return;
    }
    pull_fields();
    if (!settings_validate(&s_edit)) {
        lv_label_set_text(s_msg, "Invalid values - not saved");
        lv_obj_set_style_text_color(s_msg, HMI_COLOR_WARN, 0);
        return;
    }
    if (settings_save(&s_edit) == ESP_OK) {
        machine_cmd_send(CMD_SETTINGS_RELOAD);
        lv_label_set_text(s_msg, "Saved to NVS");
        lv_obj_set_style_text_color(s_msg, HMI_COLOR_OK, 0);
    } else {
        lv_label_set_text(s_msg, "Save failed");
        lv_obj_set_style_text_color(s_msg, HMI_COLOR_FAULT, 0);
    }
}

static void on_defaults(lv_event_t *e)
{
    (void)e;
    settings_get_defaults(&s_edit);
    hmi_show_settings();
}

void hmi_show_settings(void)
{
    hmi_timers_stop();
    s_bind_count = 0;
    s_kb = NULL;
    settings_get(&s_edit);

    lv_obj_t *scr = hmi_screen_base();
    hmi_make_header(scr, "MACHINE SETTINGS", false);

    lv_obj_t *note = lv_obj_create(scr);
    lv_obj_set_size(note, 992, 32);
    lv_obj_set_pos(note, 16, 72);
    lv_obj_set_style_bg_color(note, HMI_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(note, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(note, 5, 0);
    lv_obj_set_style_border_width(note, 0, 0);
    lv_obj_set_style_pad_all(note, 0, 0);
    lv_obj_clear_flag(note, LV_OBJ_FLAG_SCROLLABLE);
    hmi_make_label(note, "NVS SETTINGS | Durations in seconds", HMI_COLOR_MUTED,
                   &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(note, 0), 12, 9);

    s_list = lv_obj_create(scr);
    lv_obj_set_size(s_list, 992, 312);
    lv_obj_set_pos(s_list, 16, 116);
    lv_obj_set_style_bg_color(s_list, HMI_COLOR_PANEL_DEEP, 0);
    lv_obj_set_style_bg_opa(s_list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_list, HMI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(s_list, 1, 0);
    lv_obj_set_style_radius(s_list, 7, 0);
    lv_obj_set_style_pad_all(s_list, 0, 0);
    lv_obj_set_scroll_dir(s_list, LV_DIR_VER);

    add_pair(s_list, 12, "Mix (s)", &s_edit.mixing_duration_ms, true,
             "Door open (s)", &s_edit.door_open_duration_ms, true);
    add_pair(s_list, 88, "Door close (s)", &s_edit.door_close_duration_ms, true,
             "Transfer (s)", &s_edit.dough_transfer_duration_ms, true);
    add_pair(s_list, 164, "Press advance (s)", &s_edit.press_advancement_duration_ms, true,
             "Press hold (s)", &s_edit.press_hold_duration_ms, true);
    add_pair(s_list, 240, "Cutter extend (s)", &s_edit.cutter_extend_duration_ms, true,
             "Cutter return (s)", &s_edit.cutter_return_duration_ms, true);
    add_pair(s_list, 316, "Conveyor (s)", &s_edit.conveyor_duration_ms, true,
             "Flattener extend (s)", &s_edit.flattener_extend_duration_ms, true);
    add_pair(s_list, 392, "Flattener return (s)", &s_edit.flattener_return_duration_ms, true,
             "Target qty", &s_edit.target_quantity, false);

    hmi_make_label(scr, "Tap a field to edit with the numeric keypad.", HMI_COLOR_MUTED,
                   &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(scr, lv_obj_get_child_cnt(scr) - 1), 16, 438);

    lv_obj_t *save = hmi_make_btn_sized(scr, "Save", HMI_COLOR_RUN, 236, 52, HMI_COLOR_TEXT);
    lv_obj_set_pos(save, 16, 462);
    lv_obj_add_event_cb(save, on_save, LV_EVENT_CLICKED, NULL);

    lv_obj_t *defs = hmi_make_btn_sized(scr, "Restore defaults", lv_color_hex(0x26384F), 236, 52,
                                        HMI_COLOR_TEXT);
    lv_obj_set_pos(defs, 268, 462);
    lv_obj_add_event_cb(defs, on_defaults, LV_EVENT_CLICKED, NULL);

    s_msg = hmi_make_label(scr, "", HMI_COLOR_MUTED, &lv_font_montserrat_14);
    lv_obj_set_pos(s_msg, 528, 478);

    hmi_make_nav_bar(scr, HMI_NAV_SETTINGS);

    s_kb = lv_keyboard_create(scr);
    lv_keyboard_set_mode(s_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_size(s_kb, 1024, 220);
    lv_obj_align(s_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(s_kb, kb_ready_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);

    hmi_load_screen(scr);
}
