#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "hmi_timers.h"
#include "machine_drawing.h"
#include "machine_task.h"

#include "lvgl.h"
#include <stdio.h>

static lv_obj_t *s_state;
static lv_obj_t *s_qty;
static lv_obj_t *s_cycle;
static lv_obj_t *s_alarm;
static lv_obj_t *s_drawing;
static lv_obj_t *s_cmd_labels[6];
static lv_obj_t *s_clear_btn;

static void exit_run(lv_event_t *e)
{
    (void)e;
    hmi_timers_stop();
    hmi_show_auto();
}

static void cmd_evt(lv_event_t *e)
{
    machine_cmd_send((machine_cmd_id_t)(uintptr_t)lv_event_get_user_data(e));
}

static void reset_evt(lv_event_t *e)
{
    (void)e;
    machine_status_t st;
    machine_status_get(&st);
    if (st.fault != FAULT_NONE) {
        machine_cmd_send(CMD_FAULT_CLEAR);
    } else {
        machine_cmd_send(CMD_STOP);
    }
}

static void set_cmd_row(int idx, const char *text, bool on)
{
    if (s_cmd_labels[idx] == NULL) {
        return;
    }
    lv_label_set_text(s_cmd_labels[idx], text);
    lv_obj_set_style_text_color(s_cmd_labels[idx], on ? HMI_COLOR_ACCENT : HMI_COLOR_MUTED, 0);
}

static void refresh(lv_timer_t *t)
{
    (void)t;
    machine_status_t st;
    machine_status_get(&st);

    char buf[64];
    snprintf(buf, sizeof(buf), "%s", st.step_name ? st.step_name : "IDLE");
    lv_label_set_text(s_state, buf);

    snprintf(buf, sizeof(buf), "%u / %u", (unsigned)st.current_qty, (unsigned)st.target_qty);
    lv_label_set_text(s_qty, buf);

    uint32_t pct = 0;
    if (st.target_qty > 0) {
        pct = (st.current_qty * 100u) / st.target_qty;
        if (pct > 100) {
            pct = 100;
        }
    }
    snprintf(buf, sizeof(buf), "%u%%", (unsigned)pct);
    lv_label_set_text(s_cycle, buf);

    if (st.fault != FAULT_NONE) {
        lv_label_set_text(s_alarm, "FAULT");
        lv_obj_set_style_text_color(s_alarm, HMI_COLOR_FAULT, 0);
        lv_obj_clear_flag(s_clear_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_label_set_text(s_alarm, "None");
        lv_obj_set_style_text_color(s_alarm, HMI_COLOR_OK, 0);
        lv_obj_add_flag(s_clear_btn, LV_OBJ_FLAG_HIDDEN);
    }

    uint8_t m = st.commanded_outputs;
    set_cmd_row(0, (m & (1u << 0)) ? "ON" : "OFF", m & (1u << 0));
    set_cmd_row(1, (m & (1u << 1)) ? "OPEN" : "CLOSE", m & (1u << 1));
    {
        const char *press = (m & (1u << 2)) ? "FWD" : ((m & (1u << 3)) ? "REV" : "COAST");
        set_cmd_row(2, press, (m & (1u << 2)) || (m & (1u << 3)));
    }
    set_cmd_row(3, (m & (1u << 4)) ? "EXT" : "RET", m & (1u << 4));
    set_cmd_row(4, (m & (1u << 5)) ? "ON" : "OFF", m & (1u << 5));
    set_cmd_row(5, (m & (1u << 6)) ? "EXT" : "RET", m & (1u << 6));

    hmi_machine_drawing_update(s_drawing, &st);
}

void hmi_show_auto_run(void)
{
    hmi_timers_stop();
    lv_obj_t *scr = hmi_screen_base();

    /* Dual-line header — Figma Automatic Idle */
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, 1024, 56);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, HMI_COLOR_HEADER, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *accent = lv_obj_create(hdr);
    lv_obj_set_size(accent, 5, 56);
    lv_obj_set_pos(accent, 0, 0);
    lv_obj_set_style_bg_color(accent, HMI_COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(accent, 0, 0);
    lv_obj_set_style_radius(accent, 0, 0);

    hmi_make_label(hdr, "PIAYA / DOUGH FORMING", HMI_COLOR_TEXT, &lv_font_montserrat_16);
    lv_obj_set_pos(lv_obj_get_child(hdr, 1), 20, 10);
    hmi_make_label(hdr, "AUTOMATIC OPERATION", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(hdr, 2), 20, 32);

    lv_obj_t *exitb = hmi_make_btn_sized(hdr, "Exit run view", HMI_COLOR_BTN_IDLE, 140, 32,
                                         HMI_COLOR_SOFT);
    lv_obj_set_pos(exitb, 670, 12);
    lv_obj_add_event_cb(exitb, exit_run, LV_EVENT_CLICKED, NULL);

    lv_obj_t *mode = hmi_make_btn_sized(hdr, "AUTOMATIC", HMI_COLOR_RUN, 120, 24, HMI_COLOR_TEXT);
    lv_obj_set_pos(mode, 820, 16);
    hmi_make_label(hdr, "PD-01", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_align(lv_obj_get_child(hdr, lv_obj_get_child_cnt(hdr) - 1), LV_ALIGN_RIGHT_MID, -16, 0);

    /* Status strip — Figma Machine status @ y=68 */
    lv_obj_t *seq = hmi_make_card(scr, 16, 68, 424, 56);
    hmi_make_label(seq, "SEQUENCE STATUS", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(seq, 0), 12, 6);
    s_state = hmi_make_label(seq, "IDLE", HMI_COLOR_TEXT, &lv_font_montserrat_20);
    lv_obj_set_pos(s_state, 12, 24);

    lv_obj_t *prod = hmi_make_card(scr, 452, 68, 200, 56);
    hmi_make_label(prod, "BATCH / PIECES", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(prod, 0), 12, 6);
    s_qty = hmi_make_label(prod, "0 / 1", HMI_COLOR_TEXT, &lv_font_montserrat_20);
    lv_obj_set_pos(s_qty, 12, 24);

    lv_obj_t *cyc = hmi_make_card(scr, 664, 68, 180, 56);
    hmi_make_label(cyc, "FORMING CYCLE", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(cyc, 0), 12, 6);
    s_cycle = hmi_make_label(cyc, "0%", HMI_COLOR_ACCENT, &lv_font_montserrat_20);
    lv_obj_set_pos(s_cycle, 12, 24);

    lv_obj_t *alm = hmi_make_card(scr, 856, 68, 152, 56);
    hmi_make_label(alm, "ALARM", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(alm, 0), 12, 6);
    s_alarm = hmi_make_label(alm, "None", HMI_COLOR_OK, &lv_font_montserrat_16);
    lv_obj_set_pos(s_alarm, 12, 24);

    /* Exact Figma Machine visualization 736x368 */
    s_drawing = hmi_machine_drawing_create(scr, 16, 136, 736, 368, true);

    /* Machine commands — Figma 764,136 244x368 */
    lv_obj_t *cmds = hmi_make_card(scr, 764, 136, 244, 368);
    hmi_make_label(cmds, "REQUESTED OUTPUTS", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(cmds, 0), 16, 16);
    hmi_make_label(cmds, "Commanded relays only", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(cmds, 1), 16, 36);

    static const char *rows[] = {
        "Mixer", "Door", "Press", "Cutter", "Conveyor", "Flattener"
    };
    static const char *relays[] = {
        "R1", "R2", "R3/R4", "R5", "R6", "R7"
    };
    for (int i = 0; i < 6; i++) {
        lv_coord_t row_y = 65 + i * 39;
        hmi_make_label(cmds, rows[i], HMI_COLOR_SOFT, &lv_font_montserrat_14);
        lv_obj_set_pos(lv_obj_get_child(cmds, 2 + i * 3), 22, row_y + 10);
        hmi_make_label(cmds, relays[i], HMI_COLOR_MUTED, &lv_font_montserrat_12);
        lv_obj_set_pos(lv_obj_get_child(cmds, 3 + i * 3), 103, row_y + 12);
        s_cmd_labels[i] = hmi_make_label(cmds, "-", HMI_COLOR_MUTED, &lv_font_montserrat_14);
        lv_obj_set_pos(s_cmd_labels[i], 180, row_y + 10);
    }

    hmi_make_label(cmds, "No position feedback implied.", HMI_COLOR_MUTED,
                   &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(cmds, lv_obj_get_child_cnt(cmds) - 1), 16, 325);

    /* Operation controls — Figma y=516 */
    lv_obj_t *b;
    b = hmi_make_btn_sized(scr, "START", HMI_COLOR_RUN, 160, 64, HMI_COLOR_TEXT);
    lv_obj_set_pos(b, 16, 526);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_START_AUTO);
    b = hmi_make_btn_sized(scr, "PAUSE", HMI_COLOR_PAUSE_BG, 160, 64, HMI_COLOR_PAUSE_FG);
    lv_obj_set_pos(b, 188, 526);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_PAUSE);
    b = hmi_make_btn_sized(scr, "STOP", HMI_COLOR_FAULT, 160, 64, HMI_COLOR_TEXT);
    lv_obj_set_pos(b, 360, 526);
    lv_obj_add_event_cb(b, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_STOP);
    b = hmi_make_btn_sized(scr, "RESET", HMI_COLOR_RESET_BG, 160, 64, HMI_COLOR_TEXT);
    lv_obj_set_pos(b, 532, 526);
    lv_obj_add_event_cb(b, reset_evt, LV_EVENT_CLICKED, NULL);

    s_clear_btn = hmi_make_btn_sized(scr, "Clear Fault", HMI_COLOR_WARN, 296, 40, HMI_COLOR_ON_WARN);
    lv_obj_set_pos(s_clear_btn, 712, 526);
    lv_obj_add_event_cb(s_clear_btn, cmd_evt, LV_EVENT_CLICKED, (void *)(uintptr_t)CMD_FAULT_CLEAR);
    lv_obj_add_flag(s_clear_btn, LV_OBJ_FLAG_HIDDEN);

    hmi_make_label(scr, "Cylinder moves are timed.", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(scr, lv_obj_get_child_cnt(scr) - 1), 712, 575);

    refresh(NULL);
    hmi_timers_set(lv_timer_create(refresh, 200, NULL));
    hmi_load_screen(scr);
}
