#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "machine_task.h"

#include "lvgl.h"

static void on_ack(lv_event_t *e)
{
    (void)e;
    machine_cmd_send(CMD_RECOVERY_ACK);
    hmi_show_dashboard();
}

static void add_check_row(lv_obj_t *parent, lv_coord_t y, const char *num, const char *txt)
{
    lv_obj_t *n = hmi_make_label(parent, num, HMI_COLOR_WARN, &lv_font_montserrat_12);
    lv_obj_set_pos(n, 20, y);
    lv_obj_t *t = hmi_make_label(parent, txt, HMI_COLOR_TEXT, &lv_font_montserrat_16);
    lv_obj_set_pos(t, 61, y - 2);
    lv_obj_t *sep = lv_obj_create(parent);
    lv_obj_set_size(sep, 518, 1);
    lv_obj_set_pos(sep, 61, y + 28);
    lv_obj_set_style_bg_color(sep, HMI_COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_radius(sep, 0, 0);
    lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);
}

void hmi_show_recovery(void)
{
    lv_obj_t *scr = hmi_screen_base();

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
    lv_obj_set_style_bg_color(accent, HMI_COLOR_WARN, 0);
    lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(accent, 0, 0);
    lv_obj_set_style_radius(accent, 0, 0);
    lv_obj_clear_flag(accent, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *gate = hmi_make_label(hdr, "PIAYA DOUGH FORMING / BOOT GATE", HMI_COLOR_MUTED,
                                    &lv_font_montserrat_12);
    lv_obj_set_pos(gate, 24, 20);
    lv_obj_t *lock = hmi_make_label(hdr, "RECOVERY LOCKED", HMI_COLOR_WARN, &lv_font_montserrat_12);
    lv_obj_align(lock, LV_ALIGN_RIGHT_MID, -24, 0);

    lv_obj_t *title = hmi_make_label(scr, "RECOVERY REQUIRED", HMI_COLOR_WARN, &lv_font_montserrat_24);
    lv_obj_set_pos(title, 24, 79);
    lv_obj_t *instr = hmi_make_label(scr,
                                     "Inspect the machine before enabling Manual or Automatic operation.",
                                     HMI_COLOR_SOFT, &lv_font_montserrat_14);
    lv_obj_set_pos(instr, 24, 119);

    lv_obj_t *list = hmi_make_card(scr, 24, 146, 600, 312);
    lv_obj_t *lh = hmi_make_label(list, "OPERATOR INSPECTION CHECKLIST", HMI_COLOR_MUTED,
                                  &lv_font_montserrat_12);
    lv_obj_set_pos(lh, 20, 18);
    add_check_row(list, 56, "01", "E-stop released");
    add_check_row(list, 104, "02", "Air pressure checked");
    add_check_row(list, 152, "03", "Guards closed");
    add_check_row(list, 200, "04", "Press position / dough checked");
    /* last row without separator */
    lv_obj_t *n5 = hmi_make_label(list, "05", HMI_COLOR_WARN, &lv_font_montserrat_12);
    lv_obj_set_pos(n5, 20, 248);
    lv_obj_t *t5 = hmi_make_label(list, "Cutter & flattener clear", HMI_COLOR_TEXT,
                                  &lv_font_montserrat_16);
    lv_obj_set_pos(t5, 61, 246);

    lv_obj_t *guide = hmi_make_card(scr, 640, 146, 360, 312);
    lv_obj_set_style_border_color(guide, lv_color_hex(0x695125), 0);
    lv_obj_t *gh = hmi_make_label(guide, "AUTOMATIC RESTART BLOCKED", HMI_COLOR_WARN,
                                  &lv_font_montserrat_12);
    lv_obj_set_pos(gh, 20, 20);
    lv_obj_t *gb = hmi_make_label(guide,
                                  "Automatic will not resume by itself.\n\n"
                                  "Manual and Automatic remain locked until you\n"
                                  "acknowledge this inspection.",
                                  HMI_COLOR_TEXT, &lv_font_montserrat_14);
    lv_obj_set_width(gb, 316);
    lv_obj_set_pos(gb, 20, 55);
    lv_obj_t *gd = hmi_make_label(guide,
                                  "This checklist records operator acknowledgement,\n"
                                  "not sensor confirmation.",
                                  HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_width(gd, 316);
    lv_obj_set_pos(gd, 20, 239);

    lv_obj_t *btn = hmi_make_btn_sized(scr, "Machine Inspected — Continue", HMI_COLOR_WARN,
                                       976, 64, HMI_COLOR_ON_WARN);
    lv_obj_set_pos(btn, 24, 486);
    lv_obj_add_event_cb(btn, on_ack, LV_EVENT_CLICKED, NULL);

    hmi_load_screen(scr);
}
