#include "hmi_theme.h"

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
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 160, 64);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, txt);
    lv_obj_set_style_text_color(label, HMI_COLOR_TEXT, 0);
    lv_obj_center(label);
    return btn;
}

lv_obj_t *hmi_make_nav_bar(lv_obj_t *parent)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, 1024, 56);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, HMI_COLOR_PANEL, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 4, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    return bar;
}
