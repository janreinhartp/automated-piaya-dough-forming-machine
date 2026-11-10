#ifndef HMI_NAV_H
#define HMI_NAV_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *hmi_screen_base(void);
void hmi_load_screen(lv_obj_t *scr);

#ifdef __cplusplus
}
#endif

#endif /* HMI_NAV_H */
