#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "hmi_timers.h"
#include "machine_task.h"

#include "lvgl.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    machine_cmd_id_t cmd;
    bool needs_confirm;
    const char *confirm_title;
} manual_action_t;

static machine_cmd_id_t s_pending_cmd;
static lv_obj_t *s_cmd_labels[6];
static lv_timer_t *s_refresh;

static void mbox_event(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_current_target(e);
    uint16_t id = lv_btnmatrix_get_selected_btn(obj);
    const char *txt = lv_btnmatrix_get_btn_text(obj, id);
    lv_obj_t *mbox = lv_obj_get_parent(obj);
    if (txt != NULL && strcmp(txt, "Confirm") == 0) {
        machine_cmd_send(s_pending_cmd);
    }
    lv_msgbox_close(mbox);
}

static void on_action(lv_event_t *e)
{
    manual_action_t *act = (manual_action_t *)lv_event_get_user_data(e);
    if (act == NULL) {
        return;
    }
    if (!act->needs_confirm) {
        machine_cmd_send(act->cmd);
        return;
    }
    s_pending_cmd = act->cmd;
    static const char *btns[] = {"Cancel", "Confirm", ""};
    char title[48];
    snprintf(title, sizeof(title), "Confirm · %s",
             act->confirm_title ? act->confirm_title : "Motion");
    lv_obj_t *mbox = lv_msgbox_create(NULL, title,
                                      "Commanded motion — verify clear path.", btns, false);
    lv_obj_center(mbox);
    lv_obj_add_event_cb(lv_msgbox_get_btns(mbox), mbox_event, LV_EVENT_VALUE_CHANGED, NULL);
}

static manual_action_t ACT_MIX_ON = {CMD_MANUAL_MIXER_ON, false, NULL};
static manual_action_t ACT_MIX_OFF = {CMD_MANUAL_MIXER_OFF, false, NULL};
static manual_action_t ACT_DOOR_O = {CMD_MANUAL_DOOR_OPEN, true, "Door OPEN · R2"};
static manual_action_t ACT_DOOR_C = {CMD_MANUAL_DOOR_CLOSE, false, NULL};
static manual_action_t ACT_PF = {CMD_MANUAL_PRESS_FWD, true, "Press FWD · R3"};
static manual_action_t ACT_PR = {CMD_MANUAL_PRESS_REV, true, "Press REV · R4"};
static manual_action_t ACT_PC = {CMD_MANUAL_PRESS_COAST, false, NULL};
static manual_action_t ACT_CE = {CMD_MANUAL_CUTTER_EXTEND, true, "Cutter EXT · R5"};
static manual_action_t ACT_CR = {CMD_MANUAL_CUTTER_RETRACT, false, NULL};
static manual_action_t ACT_CON_ON = {CMD_MANUAL_CONVEYOR_ON, false, NULL};
static manual_action_t ACT_CON_OFF = {CMD_MANUAL_CONVEYOR_OFF, false, NULL};
static manual_action_t ACT_FE = {CMD_MANUAL_FLATTENER_EXTEND, true, "Flatten EXT · R7"};
static manual_action_t ACT_FR = {CMD_MANUAL_FLATTENER_RETRACT, false, NULL};
static manual_action_t ACT_SAFE = {CMD_MANUAL_ALL_SAFE, false, NULL};
static manual_action_t ACT_IO = {CMD_MANUAL_IO_SELFTEST, false, NULL};

static lv_obj_t *make_cmd_btn(lv_obj_t *row, const char *txt, lv_coord_t x,
                              manual_action_t *act, bool accent)
{
    lv_obj_t *btn = hmi_make_btn_sized(row, txt,
                                       accent ? HMI_COLOR_MOTION : HMI_COLOR_BTN_MUTED,
                                       112, 40,
                                       accent ? HMI_COLOR_ACCENT : lv_color_hex(0xD4DFED));
    lv_obj_set_pos(btn, x, 2);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x34445C), 0);
    lv_obj_add_event_cb(btn, on_action, LV_EVENT_CLICKED, act);
    return btn;
}

static lv_obj_t *make_row(lv_obj_t *parent, lv_coord_t y, const char *name, const char *rid)
{
    lv_obj_t *row = hmi_make_card(parent, 16, y, 992, 44);
    lv_obj_set_style_radius(row, 5, 0);
    lv_obj_t *n = hmi_make_label(row, name, HMI_COLOR_TEXT, &lv_font_montserrat_16);
    lv_obj_set_pos(n, 13, 11);
    lv_obj_t *r = hmi_make_label(row, rid, HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(r, 200, 14);
    return row;
}

static void refresh_cmd(lv_timer_t *t)
{
    (void)t;
    machine_status_t st;
    machine_status_get(&st);
    uint8_t m = st.commanded_outputs;

    const char *mix = (m & (1u << 0)) ? "CMD: ON" : "CMD: OFF";
    const char *door = (m & (1u << 1)) ? "CMD: OPEN" : "CMD: CLOSE";
    const char *press = (m & (1u << 2)) ? "CMD: FWD" : ((m & (1u << 3)) ? "CMD: REV" : "CMD: COAST");
    const char *cut = (m & (1u << 4)) ? "CMD: EXT" : "CMD: RET";
    const char *conv = (m & (1u << 5)) ? "CMD: ON" : "CMD: OFF";
    const char *flat = (m & (1u << 6)) ? "CMD: EXT" : "CMD: RET";

    if (s_cmd_labels[0]) {
        lv_label_set_text(s_cmd_labels[0], mix);
    }
    if (s_cmd_labels[1]) {
        lv_label_set_text(s_cmd_labels[1], door);
    }
    if (s_cmd_labels[2]) {
        lv_label_set_text(s_cmd_labels[2], press);
    }
    if (s_cmd_labels[3]) {
        lv_label_set_text(s_cmd_labels[3], cut);
    }
    if (s_cmd_labels[4]) {
        lv_label_set_text(s_cmd_labels[4], conv);
    }
    if (s_cmd_labels[5]) {
        lv_label_set_text(s_cmd_labels[5], flat);
    }
}

void hmi_show_manual(void)
{
    hmi_timers_stop();
    memset(s_cmd_labels, 0, sizeof(s_cmd_labels));

    lv_obj_t *scr = hmi_screen_base();
    hmi_make_header(scr, "MANUAL CONTROL", false);

    lv_obj_t *notice = lv_obj_create(scr);
    lv_obj_set_size(notice, 992, 32);
    lv_obj_set_pos(notice, 16, 72);
    lv_obj_set_style_bg_color(notice, HMI_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(notice, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(notice, 5, 0);
    lv_obj_set_style_border_width(notice, 0, 0);
    lv_obj_set_style_pad_all(notice, 0, 0);
    lv_obj_clear_flag(notice, LV_OBJ_FLAG_SCROLLABLE);
    hmi_make_label(notice, "COMMANDED OUTPUTS", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(notice, 0), 12, 9);
    lv_obj_t *rec = hmi_make_label(notice, "RECOVERY / UNLOCKED", HMI_COLOR_OK, &lv_font_montserrat_12);
    lv_obj_set_pos(rec, 278, 9);
    hmi_make_label(notice, "Verify the path before commanding motion.", HMI_COLOR_SOFT,
                   &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(notice, 2), 520, 9);

    lv_obj_t *row;
    row = make_row(scr, 120, "Mixer", "R1");
    make_cmd_btn(row, "ON", 451, &ACT_MIX_ON, false);
    make_cmd_btn(row, "OFF", 575, &ACT_MIX_OFF, false);
    s_cmd_labels[0] = hmi_make_label(row, "CMD: OFF", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(s_cmd_labels[0], 841, 14);

    row = make_row(scr, 168, "Door", "R2");
    make_cmd_btn(row, "OPEN", 451, &ACT_DOOR_O, true);
    make_cmd_btn(row, "CLOSE", 575, &ACT_DOOR_C, false);
    s_cmd_labels[1] = hmi_make_label(row, "CMD: CLOSE", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(s_cmd_labels[1], 841, 14);

    row = make_row(scr, 216, "Press", "R3 / R4");
    make_cmd_btn(row, "FWD", 451, &ACT_PF, true);
    make_cmd_btn(row, "REV", 575, &ACT_PR, true);
    make_cmd_btn(row, "COAST", 699, &ACT_PC, false);
    s_cmd_labels[2] = hmi_make_label(row, "CMD: COAST", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(s_cmd_labels[2], 841, 14);

    row = make_row(scr, 264, "Cutter", "R5");
    make_cmd_btn(row, "EXT", 451, &ACT_CE, true);
    make_cmd_btn(row, "RET", 575, &ACT_CR, false);
    s_cmd_labels[3] = hmi_make_label(row, "CMD: RET", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(s_cmd_labels[3], 841, 14);

    row = make_row(scr, 312, "Conveyor", "R6");
    make_cmd_btn(row, "ON", 451, &ACT_CON_ON, false);
    make_cmd_btn(row, "OFF", 575, &ACT_CON_OFF, false);
    s_cmd_labels[4] = hmi_make_label(row, "CMD: OFF", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(s_cmd_labels[4], 841, 14);

    row = make_row(scr, 360, "Flatten", "R7");
    make_cmd_btn(row, "EXT", 451, &ACT_FE, true);
    make_cmd_btn(row, "RET", 575, &ACT_FR, false);
    s_cmd_labels[5] = hmi_make_label(row, "CMD: RET", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(s_cmd_labels[5], 841, 14);

    lv_obj_t *safe = hmi_make_btn_sized(scr, "ALL SAFE OFF", HMI_COLOR_FAULT, 300, 56, HMI_COLOR_TEXT);
    lv_obj_set_pos(safe, 16, 424);
    lv_obj_add_event_cb(safe, on_action, LV_EVENT_CLICKED, &ACT_SAFE);

    lv_obj_t *iot = hmi_make_btn_sized(scr, "I/O SELF-TEST  R1-R8", HMI_COLOR_WARN_DARK, 300, 56,
                                       HMI_COLOR_TEXT);
    lv_obj_set_pos(iot, 332, 424);
    lv_obj_add_event_cb(iot, on_action, LV_EVENT_CLICKED, &ACT_IO);

    hmi_make_label(scr, "Motion confirmations required for OPEN, FWD, REV and EXT.",
                   HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(scr, lv_obj_get_child_cnt(scr) - 1), 656, 440);

    hmi_make_nav_bar(scr, HMI_NAV_MANUAL);
    refresh_cmd(NULL);
    s_refresh = lv_timer_create(refresh_cmd, 250, NULL);
    hmi_timers_set(s_refresh);
    hmi_load_screen(scr);
}
