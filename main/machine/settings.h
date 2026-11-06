#ifndef MACHINE_SETTINGS_H
#define MACHINE_SETTINGS_H

#include "esp_err.h"
#include "machine_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void settings_get_defaults(machine_settings_t *out);
esp_err_t settings_load(machine_settings_t *out);
esp_err_t settings_save(const machine_settings_t *in);
bool settings_validate(const machine_settings_t *in);
void settings_get(machine_settings_t *out);
esp_err_t settings_set(const machine_settings_t *in);

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_SETTINGS_H */
