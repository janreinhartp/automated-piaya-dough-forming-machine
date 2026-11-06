#ifndef MACHINE_ACTUATORS_H
#define MACHINE_ACTUATORS_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t actuators_all_safe(void);

esp_err_t actuators_mixer(bool on);
esp_err_t actuators_door_extend(bool extend);
esp_err_t actuators_press_forward(bool on);
esp_err_t actuators_press_reverse(bool on);
esp_err_t actuators_press_coast(void);
esp_err_t actuators_cutter_extend(bool extend);
esp_err_t actuators_conveyor(bool on);
esp_err_t actuators_flattener_extend(bool extend);

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_ACTUATORS_H */
