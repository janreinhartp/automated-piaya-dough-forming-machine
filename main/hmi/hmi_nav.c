#include "hmi_nav.h"
#include "hmi_theme.h"

#include "lvgl.h"

lv_obj_t *hmi_screen_base(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, HMI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    return scr;
}

void hmi_load_screen(lv_obj_t *scr)
{
    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
}

void hmi_attach_nav(lv_obj_t *bar)
{
    /* Legacy no-op: navigation is built by hmi_make_nav_bar(). */
    (void)bar;
}
