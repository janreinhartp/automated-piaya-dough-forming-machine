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

static lv_obj_t *add_field(lv_obj_t *parent, const char *name, uint32_t *field)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 960, 48);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 12, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lab = lv_label_create(row);
    lv_label_set_text(lab, name);
    lv_obj_set_style_text_color(lab, HMI_COLOR_TEXT, 0);
    lv_obj_set_width(lab, 360);

    lv_obj_t *ta = lv_textarea_create(row);
    lv_obj_set_size(ta, 200, 40);
    lv_textarea_set_one_line(ta, true);
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", (unsigned)*field);
    lv_textarea_set_text(ta, buf);

    if (s_bind_count < 12) {
        s_binds[s_bind_count].field = field;
        s_binds[s_bind_count].ta = ta;
        s_bind_count++;
    }
    return ta;
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
        return;
    }
    if (settings_save(&s_edit) == ESP_OK) {
        machine_cmd_send(CMD_SETTINGS_RELOAD);
        lv_label_set_text(s_msg, "Saved to NVS");
    } else {
        lv_label_set_text(s_msg, "Save failed");
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

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "MACHINE SETTINGS");
    lv_obj_set_style_text_color(title, HMI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 24, 8);

    lv_obj_t *list = lv_obj_create(scr);
    lv_obj_set_size(list, 980, 420);
    lv_obj_align(list, LV_ALIGN_TOP_LEFT, 20, 48);
    lv_obj_set_style_bg_color(list, HMI_COLOR_PANEL, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 2, 0);

    add_field(list, "Mix ms", &s_edit.mixing_duration_ms);
    add_field(list, "Door open ms", &s_edit.door_open_duration_ms);
    add_field(list, "Door close ms", &s_edit.door_close_duration_ms);
    add_field(list, "Transfer ms", &s_edit.dough_transfer_duration_ms);
    add_field(list, "Press advance ms", &s_edit.press_advancement_duration_ms);
    add_field(list, "Press hold ms", &s_edit.press_hold_duration_ms);
    add_field(list, "Cutter extend ms", &s_edit.cutter_extend_duration_ms);
    add_field(list, "Cutter return ms", &s_edit.cutter_return_duration_ms);
    add_field(list, "Conveyor ms", &s_edit.conveyor_duration_ms);
    add_field(list, "Flattener extend ms", &s_edit.flattener_extend_duration_ms);
    add_field(list, "Flattener return ms", &s_edit.flattener_return_duration_ms);
    add_field(list, "Target qty", &s_edit.target_quantity);

    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_set_size(row, 980, 64);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 20, 475);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 12, 0);

    lv_obj_t *b = hmi_make_btn(row, "SAVE", HMI_COLOR_RUN);
    lv_obj_add_event_cb(b, on_save, LV_EVENT_CLICKED, NULL);
    b = hmi_make_btn(row, "DEFAULTS", HMI_COLOR_PAUSE);
    lv_obj_add_event_cb(b, on_defaults, LV_EVENT_CLICKED, NULL);

    s_msg = lv_label_create(row);
    lv_label_set_text(s_msg, "");
    lv_obj_set_style_text_color(s_msg, HMI_COLOR_MUTED, 0);

    lv_obj_t *nav = hmi_make_nav_bar(scr);
    hmi_attach_nav(nav);
    hmi_load_screen(scr);
}
