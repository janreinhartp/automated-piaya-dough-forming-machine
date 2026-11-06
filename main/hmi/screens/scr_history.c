#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "history.h"

#include "lvgl.h"
#include <stdio.h>

void hmi_show_history(void)
{
    production_history_t h;
    history_get(&h);

    lv_obj_t *scr = hmi_screen_base();

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "PRODUCTION HISTORY");
    lv_obj_set_style_text_color(title, HMI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 24, 16);

    char buf[256];
    snprintf(buf, sizeof(buf),
             "Last production quantity: %u\n"
             "Total completed forming cycles: %u\n"
             "Last production duration: %u ms\n"
             "Last timestamp (unix, 0 if unavailable): %u",
             (unsigned)h.last_qty,
             (unsigned)h.total_cycles,
             (unsigned)h.last_duration_ms,
             (unsigned)h.last_unix);

    lv_obj_t *body = lv_label_create(scr);
    lv_label_set_text(body, buf);
    lv_obj_set_style_text_color(body, HMI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(body, &lv_font_montserrat_20, 0);
    lv_obj_align(body, LV_ALIGN_TOP_LEFT, 24, 100);

    lv_obj_t *nav = hmi_make_nav_bar(scr);
    hmi_attach_nav(nav);
    hmi_load_screen(scr);
}
