#ifndef HMI_MACHINE_DRAWING_H
#define HMI_MACHINE_DRAWING_H

#include "lvgl.h"
#include "machine_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Create assembly image (+ optional 01-06 station rail). Returns container. */
lv_obj_t *hmi_machine_drawing_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                     lv_coord_t w, lv_coord_t h, bool show_rail);

void hmi_machine_drawing_update(lv_obj_t *drawing, const machine_status_t *st);

#ifdef __cplusplus
}
#endif

#endif /* HMI_MACHINE_DRAWING_H */
