#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "history.h"

#include "lvgl.h"
#include <stdio.h>

static lv_obj_t *stat_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                           const char *title, const char *value, const char *unit)
{
    lv_obj_t *c = hmi_make_card(parent, x, y, 488, 134);
    lv_obj_t *t = hmi_make_label(c, title, HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(t, 17, 17);
    lv_obj_t *v = hmi_make_label(c, value, HMI_COLOR_TEXT, &lv_font_montserrat_44);
    lv_obj_set_pos(v, 17, 47);
    lv_obj_t *u = hmi_make_label(c, unit, HMI_COLOR_MUTED, &lv_font_montserrat_14);
    lv_obj_set_pos(u, 120, 67);
    return c;
}

void hmi_show_history(void)
{
    production_history_t h;
    history_get(&h);

    lv_obj_t *scr = hmi_screen_base();
    hmi_make_header(scr, "PRODUCTION HISTORY", false);

    lv_obj_t *banner = lv_obj_create(scr);
    lv_obj_set_size(banner, 992, 32);
    lv_obj_set_pos(banner, 16, 72);
    lv_obj_set_style_bg_color(banner, HMI_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(banner, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(banner, 5, 0);
    lv_obj_set_style_border_width(banner, 0, 0);
    lv_obj_set_style_pad_all(banner, 0, 0);
    lv_obj_clear_flag(banner, LV_OBJ_FLAG_SCROLLABLE);

    if (h.total_cycles == 0 && h.last_qty == 0) {
        hmi_make_label(banner, "NO COMPLETED PRODUCTION", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    } else {
        hmi_make_label(banner, "LAST COMPLETED BATCH ON RECORD", HMI_COLOR_MUTED,
                       &lv_font_montserrat_12);
    }
    lv_obj_set_pos(lv_obj_get_child(banner, 0), 12, 9);

    char buf[32];
    snprintf(buf, sizeof(buf), "%u", (unsigned)h.last_qty);
    stat_card(scr, 16, 120, "LAST PRODUCTION QUANTITY", buf, "pieces");

    snprintf(buf, sizeof(buf), "%u", (unsigned)h.total_cycles);
    stat_card(scr, 520, 120, "TOTAL COMPLETED FORMING CYCLES", buf, "cycles");

    snprintf(buf, sizeof(buf), "%u", (unsigned)h.last_duration_ms);
    stat_card(scr, 16, 270, "LAST PRODUCTION DURATION", buf, "ms");

    snprintf(buf, sizeof(buf), "%u", (unsigned)h.last_unix);
    lv_obj_t *ts = stat_card(scr, 520, 270, "LAST TIMESTAMP", buf, "unix");
    hmi_make_label(ts, "0 = no timestamp recorded", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(ts, lv_obj_get_child_cnt(ts) - 1), 202, 70);

    lv_obj_t *expl = hmi_make_card(scr, 16, 436, 992, 78);
    lv_obj_set_style_bg_color(expl, HMI_COLOR_PANEL_DEEP, 0);
    lv_obj_set_style_radius(expl, 7, 0);
    hmi_make_label(expl, "Production records appear after a completed batch.", HMI_COLOR_TEXT,
                   &lv_font_montserrat_14);
    lv_obj_set_pos(lv_obj_get_child(expl, 0), 15, 15);
    hmi_make_label(expl,
                   "Quantity, completed cycles, duration and Unix timestamp are supplied by the controller.",
                   HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(lv_obj_get_child(expl, 1), 15, 43);

    hmi_make_nav_bar(scr, HMI_NAV_HISTORY);
    hmi_load_screen(scr);
}
