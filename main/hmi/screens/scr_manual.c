#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "machine_task.h"

#include "lvgl.h"
#include <string.h>

typedef struct {
    machine_cmd_id_t cmd;
    bool needs_confirm;
} manual_action_t;

static machine_cmd_id_t s_pending_cmd;

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
    lv_obj_t *mbox = lv_msgbox_create(NULL, "Confirm", "Commanded motion — verify clear path.", btns, false);
    lv_obj_center(mbox);
    lv_obj_add_event_cb(lv_msgbox_get_btns(mbox), mbox_event, LV_EVENT_VALUE_CHANGED, NULL);
}

static manual_action_t ACT_MIX_ON = {CMD_MANUAL_MIXER_ON, false};
static manual_action_t ACT_MIX_OFF = {CMD_MANUAL_MIXER_OFF, false};
static manual_action_t ACT_DOOR_O = {CMD_MANUAL_DOOR_OPEN, true};
static manual_action_t ACT_DOOR_C = {CMD_MANUAL_DOOR_CLOSE, false};
static manual_action_t ACT_PF = {CMD_MANUAL_PRESS_FWD, true};
static manual_action_t ACT_PR = {CMD_MANUAL_PRESS_REV, true};
static manual_action_t ACT_PC = {CMD_MANUAL_PRESS_COAST, false};
static manual_action_t ACT_CE = {CMD_MANUAL_CUTTER_EXTEND, true};
static manual_action_t ACT_CR = {CMD_MANUAL_CUTTER_RETRACT, false};
static manual_action_t ACT_CON_ON = {CMD_MANUAL_CONVEYOR_ON, false};
static manual_action_t ACT_CON_OFF = {CMD_MANUAL_CONVEYOR_OFF, false};
static manual_action_t ACT_FE = {CMD_MANUAL_FLATTENER_EXTEND, true};
static manual_action_t ACT_FR = {CMD_MANUAL_FLATTENER_RETRACT, false};
static manual_action_t ACT_SAFE = {CMD_MANUAL_ALL_SAFE, false};
static manual_action_t ACT_IO_TEST = {CMD_MANUAL_IO_SELFTEST, false};

static void add_pair(lv_obj_t *parent, const char *a, manual_action_t *aa, const char *b, manual_action_t *bb)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, 960, 64);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 12, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn = hmi_make_btn(row, a, HMI_COLOR_ACCENT);
    lv_obj_add_event_cb(btn, on_action, LV_EVENT_CLICKED, aa);
    btn = hmi_make_btn(row, b, HMI_COLOR_STOP);
    lv_obj_add_event_cb(btn, on_action, LV_EVENT_CLICKED, bb);
}

void hmi_show_manual(void)
{
    lv_obj_t *scr = hmi_screen_base();

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "MANUAL CONTROL (commanded outputs)");
    lv_obj_set_style_text_color(title, HMI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 24, 12);

    lv_obj_t *col = lv_obj_create(scr);
    lv_obj_set_size(col, 980, 470);
    lv_obj_align(col, LV_ALIGN_TOP_LEFT, 20, 50);
    lv_obj_set_style_bg_color(col, HMI_COLOR_PANEL, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col, 4, 0);

    add_pair(col, "Mixer ON", &ACT_MIX_ON, "Mixer OFF", &ACT_MIX_OFF);
    add_pair(col, "Door OPEN", &ACT_DOOR_O, "Door CLOSE", &ACT_DOOR_C);
    add_pair(col, "Press FWD", &ACT_PF, "Press REV", &ACT_PR);

    lv_obj_t *coast = hmi_make_btn(col, "Press COAST", HMI_COLOR_PAUSE);
    lv_obj_add_event_cb(coast, on_action, LV_EVENT_CLICKED, &ACT_PC);

    add_pair(col, "Cutter EXT", &ACT_CE, "Cutter RET", &ACT_CR);
    add_pair(col, "Conveyor ON", &ACT_CON_ON, "Conveyor OFF", &ACT_CON_OFF);
    add_pair(col, "Flatten EXT", &ACT_FE, "Flatten RET", &ACT_FR);

    lv_obj_t *safe = hmi_make_btn(col, "ALL SAFE OFF", HMI_COLOR_FAULT);
    lv_obj_add_event_cb(safe, on_action, LV_EVENT_CLICKED, &ACT_SAFE);

    lv_obj_t *iotest = hmi_make_btn(col, "I/O SELF-TEST R1-R8", HMI_COLOR_WARN);
    lv_obj_set_width(iotest, 280);
    lv_obj_add_event_cb(iotest, on_action, LV_EVENT_CLICKED, &ACT_IO_TEST);

    lv_obj_t *nav = hmi_make_nav_bar(scr);
    hmi_attach_nav(nav);
    hmi_load_screen(scr);
}
