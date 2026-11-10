#include "hmi_theme.h"
#include "hmi_init.h"
#include "hmi_timers.h"
#include "safety.h"

void hmi_theme_apply(void)
{
    lv_disp_t *disp = lv_disp_get_default();
    if (disp == NULL) {
        return;
    }
    lv_theme_t *th = lv_theme_default_init(disp, HMI_COLOR_ACCENT, HMI_COLOR_WARN,
                                           true, LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, th);
}

lv_obj_t *hmi_make_btn(lv_obj_t *parent, const char *txt, lv_color_t bg)
{
    return hmi_make_btn_sized(parent, txt, bg, 160, 64, HMI_COLOR_TEXT);
}

lv_obj_t *hmi_make_btn_sized(lv_obj_t *parent, const char *txt, lv_color_t bg,
                             lv_coord_t w, lv_coord_t h, lv_color_t fg)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, txt);
    lv_obj_set_style_text_color(label, fg, 0);
    lv_obj_center(label);
    return btn;
}

lv_obj_t *hmi_make_label(lv_obj_t *parent, const char *txt, lv_color_t color,
                         const lv_font_t *font)
{
    lv_obj_t *lab = lv_label_create(parent);
    lv_label_set_text(lab, txt);
    lv_obj_set_style_text_color(lab, color, 0);
    if (font != NULL) {
        lv_obj_set_style_text_font(lab, font, 0);
    }
    return lab;
}

lv_obj_t *hmi_make_header(lv_obj_t *parent, const char *title, bool warn_accent)
{
    lv_obj_t *hdr = lv_obj_create(parent);
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
    lv_obj_set_style_bg_color(accent, warn_accent ? HMI_COLOR_WARN : HMI_COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(accent, 0, 0);
    lv_obj_set_style_radius(accent, 0, 0);
    lv_obj_clear_flag(accent, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = hmi_make_label(hdr, title, HMI_COLOR_TEXT, &lv_font_montserrat_20);
    lv_obj_set_pos(t, 24, 16);

    lv_obj_t *prev = hmi_make_label(hdr, "PD-01", HMI_COLOR_MUTED, &lv_font_montserrat_12);
    lv_obj_align(prev, LV_ALIGN_RIGHT_MID, -24, 0);

    lv_obj_t *rule = lv_obj_create(hdr);
    lv_obj_set_size(rule, 1024, 1);
    lv_obj_set_pos(rule, 0, 55);
    lv_obj_set_style_bg_color(rule, HMI_COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rule, 0, 0);
    lv_obj_set_style_radius(rule, 0, 0);

    return hdr;
}

lv_obj_t *hmi_make_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                        lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, HMI_COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, HMI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

static void nav_go(lv_event_t *e)
{
    hmi_nav_id_t id = (hmi_nav_id_t)(uintptr_t)lv_event_get_user_data(e);
    hmi_timers_stop();
    switch (id) {
    case HMI_NAV_DASHBOARD:
        hmi_show_dashboard();
        break;
    case HMI_NAV_MANUAL:
        if (!safety_is_recovery_unlocked()) {
            return;
        }
        hmi_show_manual();
        break;
    case HMI_NAV_AUTO:
        if (!safety_is_recovery_unlocked()) {
            return;
        }
        hmi_show_auto();
        break;
    case HMI_NAV_SETTINGS:
        hmi_show_settings();
        break;
    case HMI_NAV_HISTORY:
        hmi_show_history();
        break;
    }
}

lv_obj_t *hmi_make_nav_bar(lv_obj_t *parent, hmi_nav_id_t active)
{
    static const char *labels[] = {
        "Dashboard", "Manual", "Automatic", "Settings", "History"
    };

    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, 1024, 64);
    lv_obj_set_pos(bar, 0, 536);
    lv_obj_set_style_bg_color(bar, HMI_COLOR_HEADER, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *rule = lv_obj_create(bar);
    lv_obj_set_size(rule, 1024, 1);
    lv_obj_set_pos(rule, 0, 0);
    lv_obj_set_style_bg_color(rule, HMI_COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rule, 0, 0);
    lv_obj_set_style_radius(rule, 0, 0);

    for (int i = 0; i < 5; i++) {
        lv_coord_t x = 12 + i * 200;
        bool sel = ((hmi_nav_id_t)i == active);
        lv_obj_t *tab = lv_btn_create(bar);
        lv_obj_set_size(tab, 200, 60);
        lv_obj_set_pos(tab, x, 4);
        lv_obj_set_style_radius(tab, 4, 0);
        lv_obj_set_style_border_width(tab, 0, 0);
        lv_obj_set_style_shadow_width(tab, 0, 0);
        lv_obj_set_style_bg_color(tab, sel ? HMI_COLOR_NAV_ACTIVE : HMI_COLOR_HEADER, 0);
        lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);
        lv_obj_add_event_cb(tab, nav_go, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        if (sel) {
            lv_obj_t *ind = lv_obj_create(tab);
            lv_obj_set_size(ind, 152, 3);
            lv_obj_align(ind, LV_ALIGN_TOP_MID, 0, 0);
            lv_obj_set_style_bg_color(ind, HMI_COLOR_ACCENT, 0);
            lv_obj_set_style_bg_opa(ind, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(ind, 0, 0);
            lv_obj_set_style_radius(ind, 1, 0);
            lv_obj_clear_flag(ind, LV_OBJ_FLAG_SCROLLABLE);
        }

        lv_obj_t *lab = lv_label_create(tab);
        lv_label_set_text(lab, labels[i]);
        lv_obj_set_style_text_color(lab, sel ? HMI_COLOR_ACCENT : HMI_COLOR_SOFT, 0);
        lv_obj_center(lab);
    }

    return bar;
}
