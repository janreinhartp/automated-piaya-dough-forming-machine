#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "machine_task.h"
#include "settings.h"

#include "esp_err.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>

static machine_settings_t s_edit;
static lv_obj_t *s_msg;

typedef struct {
    uint32_t *field;
    lv_obj_t *ta;
} field_bind_t;

static field_bind_t s_binds[12];
static int s_bind_count;

static void add_pair(lv_obj_t *parent, lv_coord_t y, const char *lname, uint32_t *lfield,
                     const char *rname, uint32_t *rfield, const char *runit)
{
    lv_obj_t *ll = hmi_make_label(parent, lname, HMI_COLOR_SOFT, &lv_font_montserrat_12);
    lv_obj_set_pos(ll, 16, y);

    lv_obj_t *lta = lv_textarea_create(parent);
    lv_obj_set_size(lta, 452, 40);
    lv_obj_set_pos(lta, 16, y + 22);
    lv_textarea_set_one_line(lta, true);
    lv_obj_set_style_bg_color(lta, HMI_COLOR_FIELD, 0);
    lv_obj_set_style_border_color(lta, HMI_COLOR_FIELD_BD, 0);
    lv_obj_set_style_border_width(lta, 1, 0);
    lv_obj_set_style_radius(lta, 5, 0);
    lv_obj_set_style_text_color(lta, HMI_COLOR_TEXT, 0);
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", (unsigned)*lfield);
    lv_textarea_set_text(lta, buf);
    if (s_bind_count < 12) {
        s_binds[s_bind_count].field = lfield;
        s_binds[s_bind_count].ta = lta;
        s_bind_count++;
    }

    lv_obj_t *rl = hmi_make_label(parent, rname, HMI_COLOR_SOFT, &lv_font_montserrat_12);
    lv_obj_set_pos(rl, 486, y);

    lv_obj_t *rta = lv_textarea_create(parent);
    lv_obj_set_size(rta, 452, 40);
    lv_obj_set_pos(rta, 486, y + 22);
    lv_textarea_set_one_line(rta, true);
    lv_obj_set_style_bg_color(rta, HMI_COLOR_FIELD, 0);
    lv_obj_set_style_border_color(rta, HMI_COLOR_FIELD_BD, 0);
    lv_obj_set_style_border_width(rta, 1, 0);
    lv_obj_set_style_radius(rta, 5, 0);
    lv_obj_set_style_text_color(rta, HMI_COLOR_TEXT, 0);
    snprintf(buf, sizeof(buf), "%u", (unsigned)*rfield);
    lv_textarea_set_text(rta, buf);
    if (s_bind_count < 12) {
        s_binds[s_bind_count].field = rfield;
        s_binds[s_bind_count].ta = rta;
        s_bind_count++;
    }
    (void)runit;
}

static void pull_fields(void)
{
    for (int i = 0; i < s_bind_count; i++) {
        const char *txt = lv_textarea_get_text(s_binds[i].ta);
        *s_binds[i].field = (uint32_t)strtoul(txt, NULL, 10);
    }
}

static void on_save(lv_event_t *e)
{
    (void)e;
    pull_fields();
    if (!settings_validate(&s_edit)) {
        lv_label_set_text(s_msg, "Invalid values — not saved");
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
    s_bind_count = 0;
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
    hmi_make_label(note, "NVS SETTINGS · Durations in ms", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(note, 0), 12, 9);

    lv_obj_t *list = lv_obj_create(scr);
    lv_obj_set_size(list, 992, 312);
    lv_obj_set_pos(list, 16, 116);
    lv_obj_set_style_bg_color(list, HMI_COLOR_PANEL_DEEP, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(list, HMI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(list, 1, 0);
    lv_obj_set_style_radius(list, 7, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    add_pair(list, 12, "Mix (ms)", &s_edit.mixing_duration_ms,
             "Door open (ms)", &s_edit.door_open_duration_ms, "ms");
    add_pair(list, 88, "Door close (ms)", &s_edit.door_close_duration_ms,
             "Transfer (ms)", &s_edit.dough_transfer_duration_ms, "ms");
    add_pair(list, 164, "Press advance (ms)", &s_edit.press_advancement_duration_ms,
             "Press hold (ms)", &s_edit.press_hold_duration_ms, "ms");
    add_pair(list, 240, "Cutter extend (ms)", &s_edit.cutter_extend_duration_ms,
             "Cutter return (ms)", &s_edit.cutter_return_duration_ms, "ms");
    add_pair(list, 316, "Conveyor (ms)", &s_edit.conveyor_duration_ms,
             "Flattener extend (ms)", &s_edit.flattener_extend_duration_ms, "ms");
    add_pair(list, 392, "Flattener return (ms)", &s_edit.flattener_return_duration_ms,
             "Target qty", &s_edit.target_quantity, "pcs");

    hmi_make_label(scr, "Scroll for remaining timings and Target qty.", HMI_COLOR_MUTED,
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
    hmi_load_screen(scr);
}
