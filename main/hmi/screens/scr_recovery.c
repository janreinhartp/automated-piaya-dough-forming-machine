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

void hmi_show_recovery(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, HMI_COLOR_BG, 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "RECOVERY REQUIRED");
    lv_obj_set_style_text_color(title, HMI_COLOR_WARN, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *body = lv_label_create(scr);
    lv_label_set_text(body,
                      "Power was restored (E-stop or power loss).\n\n"
                      "Before continuing, inspect:\n"
                      "  - E-stop released and machine powered safely\n"
                      "  - Air pressure available\n"
                      "  - Guards closed\n"
                      "  - Press chamber position and remaining dough\n"
                      "  - Cutter and flattener clear\n\n"
                      "Automatic production will NOT resume by itself.\n"
                      "Manual and Auto stay locked until you acknowledge.");
    lv_obj_set_style_text_color(body, HMI_COLOR_TEXT, 0);
    lv_obj_set_width(body, 900);
    lv_obj_align(body, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *btn = hmi_make_btn(scr, "Machine Inspected — Continue", HMI_COLOR_WARN);
    lv_obj_set_size(btn, 420, 72);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_add_event_cb(btn, on_ack, LV_EVENT_CLICKED, NULL);

    hmi_load_screen(scr);
}
