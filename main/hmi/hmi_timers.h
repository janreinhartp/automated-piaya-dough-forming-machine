#ifndef HMI_TIMERS_H
#define HMI_TIMERS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void hmi_timers_stop(void);
void hmi_timers_set(lv_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif /* HMI_TIMERS_H */
