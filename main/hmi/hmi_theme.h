#ifndef HMI_THEME_H
#define HMI_THEME_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HMI_COLOR_BG        lv_color_hex(0x1A1D23)
#define HMI_COLOR_PANEL     lv_color_hex(0x242830)
#define HMI_COLOR_TEXT      lv_color_hex(0xE8EAED)
#define HMI_COLOR_MUTED     lv_color_hex(0x9AA0A6)
#define HMI_COLOR_ACCENT    lv_color_hex(0x3D8BFF)
#define HMI_COLOR_RUN       lv_color_hex(0x2E7D32)
#define HMI_COLOR_STOP      lv_color_hex(0x5F6368)
#define HMI_COLOR_PAUSE     lv_color_hex(0xF9A825)
#define HMI_COLOR_FAULT     lv_color_hex(0xC62828)
#define HMI_COLOR_WARN      lv_color_hex(0xEF6C00)

void hmi_theme_apply(void);
lv_obj_t *hmi_make_btn(lv_obj_t *parent, const char *txt, lv_color_t bg);
lv_obj_t *hmi_make_nav_bar(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* HMI_THEME_H */
