#ifndef MACHINE_RELAYS_H
#define MACHINE_RELAYS_H

#include <stdbool.h>
#include "esp_err.h"
#include "machine_types.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t relays_init(uint8_t pcf_addr, bool active_low);
esp_err_t relays_set_polarity(bool active_low);
esp_err_t relays_force_safe(void);
esp_err_t relays_set(relay_id_t id, bool logical_on);
bool relays_get(relay_id_t id);
uint8_t relays_get_commanded_mask(void);
esp_err_t relays_apply_mask(uint8_t logical_on_mask);

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_RELAYS_H */
