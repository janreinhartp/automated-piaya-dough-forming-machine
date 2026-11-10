#ifndef HMI_THEME_H
#define HMI_THEME_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Figma Piaya Dough Maker tokens */
#define HMI_COLOR_BG         lv_color_hex(0x101827)
#define HMI_COLOR_HEADER     lv_color_hex(0x111C2D)
#define HMI_COLOR_PANEL      lv_color_hex(0x172235)
#define HMI_COLOR_PANEL_DEEP lv_color_hex(0x131E2F)
#define HMI_COLOR_BORDER     lv_color_hex(0x2B3A50)
#define HMI_COLOR_BTN_IDLE   lv_color_hex(0x1B293D)
#define HMI_COLOR_BTN_MUTED  lv_color_hex(0x2B3E55)
#define HMI_COLOR_TEXT       lv_color_hex(0xF1F5F9)
#define HMI_COLOR_MUTED      lv_color_hex(0xA6B6CA)
#define HMI_COLOR_SOFT       lv_color_hex(0xB7C5D8)
#define HMI_COLOR_ACCENT     lv_color_hex(0x22D3EE)
#define HMI_COLOR_NAV_ACTIVE lv_color_hex(0x163342)
#define HMI_COLOR_RUN        lv_color_hex(0x15803D)
#define HMI_COLOR_OK         lv_color_hex(0x22C55E)
#define HMI_COLOR_PAUSE_BG   lv_color_hex(0x4D3A18)
#define HMI_COLOR_PAUSE_FG   lv_color_hex(0xD5B26D)
#define HMI_COLOR_RESUME_BG  lv_color_hex(0x1D3657)
#define HMI_COLOR_RESUME_FG  lv_color_hex(0x9BBBE7)
#define HMI_COLOR_FAULT      lv_color_hex(0xB91C1C)
#define HMI_COLOR_WARN       lv_color_hex(0xF97316)
#define HMI_COLOR_WARN_DARK  lv_color_hex(0x9A4D0A)
#define HMI_COLOR_ON_WARN    lv_color_hex(0x241500)
#define HMI_COLOR_READY_BG   lv_color_hex(0x132D2C)
#define HMI_COLOR_FIELD      lv_color_hex(0x1B293D)
#define HMI_COLOR_FIELD_BD   lv_color_hex(0x3B4E69)
#define HMI_COLOR_MOTION     lv_color_hex(0x173B49)

typedef enum {
    HMI_NAV_DASHBOARD = 0,
    HMI_NAV_MANUAL,
    HMI_NAV_AUTO,
    HMI_NAV_SETTINGS,
    HMI_NAV_HISTORY
} hmi_nav_id_t;

void hmi_theme_apply(void);

lv_obj_t *hmi_make_btn(lv_obj_t *parent, const char *txt, lv_color_t bg);
lv_obj_t *hmi_make_btn_sized(lv_obj_t *parent, const char *txt, lv_color_t bg,
                             lv_coord_t w, lv_coord_t h, lv_color_t fg);
lv_obj_t *hmi_make_header(lv_obj_t *parent, const char *title, bool warn_accent);
lv_obj_t *hmi_make_nav_bar(lv_obj_t *parent, hmi_nav_id_t active);
lv_obj_t *hmi_make_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                        lv_coord_t w, lv_coord_t h);
lv_obj_t *hmi_make_label(lv_obj_t *parent, const char *txt, lv_color_t color,
                         const lv_font_t *font);

#ifdef __cplusplus
}
#endif

#endif /* HMI_THEME_H */
