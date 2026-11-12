#include "hmi_init.h"
#include "hmi_nav.h"
#include "hmi_theme.h"
#include "hmi_timers.h"
#include "history.h"

#include "lvgl.h"
#include <stdio.h>

static lv_obj_t *s_banner;
static lv_obj_t *s_qty;
static lv_obj_t *s_cycles;
static lv_obj_t *s_dur;
static lv_obj_t *s_ts;

static void format_duration(char *buf, size_t n, uint32_t ms)
{
    uint32_t sec = ms / 1000u;
    uint32_t m = sec / 60u;
    uint32_t s = sec % 60u;
    if (m > 0) {
        snprintf(buf, n, "%lu:%02lu", (unsigned long)m, (unsigned long)s);
    } else {
        snprintf(buf, n, "%lu s", (unsigned long)s);
    }
}

static void refresh(lv_timer_t *t)
{
    (void)t;
    production_history_t h;
    history_get(&h);

    if (s_banner) {
        lv_label_set_text(s_banner,
                          (h.total_cycles == 0 && h.last_qty == 0)
                              ? "NO COMPLETED PRODUCTION"
                              : "LAST COMPLETED BATCH ON RECORD");
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%u", (unsigned)h.last_qty);
    if (s_qty) {
        lv_label_set_text(s_qty, buf);
    }
    snprintf(buf, sizeof(buf), "%u", (unsigned)h.total_cycles);
    if (s_cycles) {
        lv_label_set_text(s_cycles, buf);
    }
    format_duration(buf, sizeof(buf), h.last_duration_ms);
    if (s_dur) {
        lv_label_set_text(s_dur, buf);
    }
    snprintf(buf, sizeof(buf), "%u", (unsigned)h.last_unix);
    if (s_ts) {
        lv_label_set_text(s_ts, buf);
    }
}

static lv_obj_t *stat_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                           const char *title, lv_obj_t **value_out, const char *unit)
{
    lv_obj_t *c = hmi_make_card(parent, x, y, 488, 134);
    lv_obj_t *t = hmi_make_label(c, title, HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_set_pos(t, 17, 17);
    lv_obj_t *v = hmi_make_label(c, "0", HMI_COLOR_TEXT, &lv_font_montserrat_44);
    lv_obj_set_pos(v, 17, 47);
    *value_out = v;
    lv_obj_t *u = hmi_make_label(c, unit, HMI_COLOR_MUTED, &lv_font_montserrat_14);
    lv_obj_set_pos(u, 120, 67);
    return c;
}

void hmi_show_history(void)
{
    hmi_timers_stop();
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
    s_banner = hmi_make_label(banner, "NO COMPLETED PRODUCTION", HMI_COLOR_MUTED,
                              &lv_font_montserrat_12);
    lv_obj_set_pos(s_banner, 12, 9);

    stat_card(scr, 16, 120, "LAST PRODUCTION QUANTITY", &s_qty, "pieces");
    stat_card(scr, 520, 120, "TOTAL COMPLETED FORMING CYCLES", &s_cycles, "cycles");
    stat_card(scr, 16, 270, "LAST PRODUCTION DURATION", &s_dur, "mm:ss");
    lv_obj_t *ts = stat_card(scr, 520, 270, "LAST TIMESTAMP", &s_ts, "unix");
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
    refresh(NULL);
    hmi_timers_set(lv_timer_create(refresh, 500, NULL));
    hmi_load_screen(scr);
}
